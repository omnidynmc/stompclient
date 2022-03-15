#include <fstream>
#include <string>
#include <queue>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <fstream>

#include <errno.h>
#include <limits.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>

#include <openframe/openframe.h>

#include "StompMessage.h"
#include "StompServer.h"
#include "StompPeer.h"
#include "Transaction.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** StompServer Class                                                    **
 **************************************************************************/
  const time_t StompServer::kDefaultLogstatsInterval 	= 3600;
  const size_t StompServer::kDefaultMaxWork 		= 100;
  const time_t StompServer::kDefaultQueueMessageExpire	= 3600;

  StompServer::StompServer(const int port, const int max, const std::string &bind_ip)
              : ListenController(port, max, bind_ip),
                _debug(false),
                _intval_logstats(kDefaultLogstatsInterval),
                _time_queue_expire(kDefaultQueueMessageExpire),
                _max_work(kDefaultMaxWork),
                _queue_byte_limit(Exchange::kDefaultByteLimit) {

    init_stats(true);
    _exch_manager = new ExchangeManager;
    return;
  } // StompServer::StompServer

  StompServer::~StompServer() {
    _exch_manager->release();

    _peers_l.Lock();
    for(peers_itr itr = _peers.begin(); itr != _peers.end(); itr++) {
      StompPeer *peer = itr->second;
      if (peer->refcount() > 1)
        LOG(LogWarn, << peer << " still has " << peer->refcount() << " references" << std::endl);
      peer->release();
    } // for
    _peers_l.Unlock();

    onDestroyStats();
    clear_queues();
    return;
  } // StompServer::~StompServer

  void StompServer::init_stats(const bool startup) {
    if (startup) {
      _stats.num_peers = 0;
      _stats.last_report_peers = time(NULL);
    } // if
  } // StompServer::init_stats

  void StompServer::onDescribeStats() {
    set_stat_id_prefix("libstomp.stompserver");
    describe_stat("num.peers", "main/num peers", openstats::graphTypeGauge, openstats::dataTypeInt, openstats::useTypeMean);
    describe_stat("time.run", "main/loop run time", openstats::graphTypeGauge, openstats::dataTypeFloat, openstats::useTypeMean);
  } // StompServer::onDescribeStats

  void StompServer::onDestroyStats() {
    destroy_stat("libstomp.*");
  } // StompServer::onDestroyStats

  const bool StompServer::run() {
    bool didWork = false;

    openframe::Stopwatch sw;
    // exqueue_queues();
    // exqueue_topics();

    sw.Start();

    openframe::scoped_lock slock(&_peers_l);

    // ### Process Peers ###
    if (_stats.last_report_peers < time(NULL) - 1) {
      datapoint("num.peers", _stats.num_peers);
      _stats.last_report_peers = time(NULL);
    } // if

    didWork = _process_peers(_peers);

    // ### Dispatch to Peers ###
    static time_t last_dispatch = time(NULL);
    const time_t dispatch_intval = 0;

    if (last_dispatch <= time(NULL) - dispatch_intval) {
      _exch_manager->dispatch_exchanges();	// FIXME
      last_dispatch = time(NULL);
    } // if

    datapoint_float("time.run", sw.Time() );

    return didWork;
  } // StompServer::run

  StompServer &StompServer::start() {
    _exch_manager->elogger( elogger(), elog_name() );
    _exch_manager->replace_stats(stats(), "stompserver.exchanges");
    super::start();
    return *this;
  } // StompServer::start

  bool StompServer::_process_peers(peers_t &peers) {
    int didWork = 0;

    std::deque<StompPeer *> discList;
    for(peers_itr ptr = peers.begin(); ptr != peers.end(); ptr++) {
      StompPeer *peer = ptr->second;

      // skip peers we're disconnecting
      if ( peer->disconnect() ) continue;

      if ( _process_peer(peer) ) didWork++;
      if ( peer->is_heart_beat_timeout() ) {
        LOG(LogNotice, << "Heart-beat timed out; " << peer << std::endl);
        peer->disconnect_with_error("Heart-beat timed out");
      } // if
//      if ( peer->disconnect() ) discList.push_back(peer);
    } // for

//    while( !discList.empty() ) {
//      StompPeer *peer = discList.front();
//      disconnect( peer->sock() );
//      discList.pop_front();
//    } // while

    return didWork;
  } // StompServer::_process_peers

  bool StompServer::_process_peer(StompPeer *peer) {
    bool didWork = false;

    //for(size_t i=0; i < _max_work && peer->process(); i++); <-- peer does this automagically now
    for(size_t i=0; i < _max_work; i++) {
      StompFrame *frame;
      bool ok = peer->next_frame(frame);
      peer->checkLogstats();
      if (!ok) break;
      didWork |= ok;
      _process(peer, frame);
      frame->release();
    } // for

    return didWork;
  } // StompServer::_process_peer

  void StompServer::_send_queues(StompPeer *peer) {
    std::deque<queues_itr> rm;
    for(queues_itr itr = _queues.begin(); itr != _queues.end(); itr++) {
      StompMessage *smesg = (*itr);
      Subscription *ssub;
      bool ok = peer->is_match( smesg->destination(), ssub );
      if (!ok || ssub->sentq() > 1024) continue;
      ssub->enqueue(smesg);
      rm.push_back(itr);
    } // for

    while( !rm.empty() ) {
      StompMessage *smesg = *rm.front();
      _queues.erase(smesg);
      rm.pop_front();
    } // while
  } // StompServer::_send_queues

  void StompServer::_process(StompPeer *peer, StompFrame *frame) {
    bool connect_only = !peer->authenticated()
                        && frame->type() != StompFrame::commandConnect;

    if (connect_only) {
      peer->disconnect_with_error("must send CONNECT first");
      return;
    } // if

    // intercept SEND and ACK in case of transactions
    bool is_intercept_trans = _store_transaction(peer, frame);
    if (is_intercept_trans) return;

    switch( frame->type() ) {
      case StompFrame::commandAck:
        _process_ack(peer, frame);
        break;
      case StompFrame::commandNack:
        _process_nack(peer, frame);
        break;
      case StompFrame::commandBegin:
        _process_begin(peer, frame);
        break;
      case StompFrame::commandAbort:
        _process_abort(peer, frame);
        break;
      case StompFrame::commandCommit:
        _process_commit(peer, frame);
        break;
      case StompFrame::commandConnect:
        _process_connect(peer, frame);
        break;
      case StompFrame::commandSend:
        _process_send(peer, frame);
        break;
      case StompFrame::commandSubscribe:
        _process_subscribe(peer, frame);
        break;
      case StompFrame::commandUnsubscribe:
        _process_unsubscribe(peer, frame);
        break;
      case StompFrame::commandDisconnect:
        _process_disconnect(peer, frame);
        break;
      default:
        peer->send_error("unknown command; " + openframe::StringTool::safe(frame->command() ) );
        break;
    } // switch

    if (!frame->is_command(StompFrame::commandConnect)
        && frame->is_header("receipt")) {
      StompFrame *receipt_frame = new StompFrame("RECEIPT");
      receipt_frame->add_header("receipt-id", frame->get_header("receipt"));
      peer->send_frame(receipt_frame);
      receipt_frame->release();
    } // if

    _process_receipt_id(peer, frame);
  } // StompServer::process

  void StompServer::_process_connect(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("login")) {
      peer->disconnect_with_error("missing login");
      return;
    } // if

    if (!frame->is_header("passcode")) {
      peer->disconnect_with_error("missing passcode");
      return;
    } // if

    std::string login = frame->get_header("login");
    std::string passcode = frame->get_header("passcode");

    if (!login.length()) {
      peer->disconnect_with_error("must specify login");
      return;
    } // if

    peer->auth(login, passcode);

    LOG(LogInfo, << "Authenticated "
                 << login
                 << "@" << peer
                 << std::endl);

    stompHeader_t headers;

    // handle heart-beat
    time_t i_will = 0;
    time_t i_support = 0;
    time_t i_expect = 0;
    if (frame->is_header("heart-beat")) {
      openframe::StringToken st;
      st.setDelimiter(',');
      st = frame->get_header("heart-beat");

      if (st.size() != 2) {
        peer->disconnect_with_error("unable to parse heart-beat header must be <cx>,<cy>");
        return;
      } // if

      time_t peer_supports = atoi( st[0].c_str() );
      time_t peer_wants = atoi( st[1].c_str() );

      if (peer_supports) i_expect = MAX(peer_supports, 5000);
      if (peer_wants) i_support = i_will = MAX(peer_wants, 1000);

      peer->enable_heart_beat(i_expect, i_will);
    } // if

    std::stringstream s;
    s << i_support << "," << i_expect;

    LOG(LogInfo, << "Heart-beat set to "
                 << i_support << "," << i_expect
                 << " for " << peer << std::endl);

    peer->connected(StompMessage::create_uuid(), new StompHeaders("heart-beat", s.str() ));
  } // StompServer::_process_connect

  void StompServer::_process_begin(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("transaction")) {
      peer->send_error("missing transaction header");
      return;
    } // if

    std::string transaction_id = frame->get_header("transaction");
    peer->create_transaction(transaction_id);
  } // StompServer::_process_begin

  void StompServer::_process_commit(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("transaction")) {
      peer->send_error("missing transaction header");
      return;
    } // if

    std::string transaction_id = frame->get_header("transaction");
    bool ok = peer->commit_transaction(transaction_id);
    if (!ok) {
      peer->send_error("transaction not found");
      return;
    } // if
  } // StompServer::_process_commit

  void StompServer::_process_abort(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("transaction")) {
      peer->send_error("missing transaction header");
      return;
    } // if

    std::string transaction_id = frame->get_header("transaction");
    bool ok = peer->destroy_transaction(transaction_id);

    if (!ok) {
      peer->send_error("transaction not found");
      return;
    } // if
  } // StompServer::_process_abort

  void StompServer::_process_ack(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("message-id")) {
      peer->send_error("ack missing message-id");
      return;
    } // if

    if (!frame->is_header("subscription")) {
      peer->send_error("ack missing subscription");
      return;
    } // if

    std::string message_id = frame->get_header("message-id");
    std::string subscription = frame->get_header("subscription");

    peer->received_ack(subscription, message_id);
  } // StompServer::_process_ack

  void StompServer::_process_nack(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("message-id")) {
      peer->send_error("ack missing message-id");
      return;
    } // if

    if (!frame->is_header("subscription")) {
      peer->send_error("ack missing subscription");
      return;
    } // if

    std::string message_id = frame->get_header("message-id");
    std::string subscription = frame->get_header("subscription");

    peer->received_nack(subscription, message_id);
  } // StompServer::_process_nack

  bool StompServer::_store_transaction(StompPeer *peer, StompFrame *frame) {
    bool is_intercept_trans = (frame->is_command(StompFrame::commandAck) || frame->is_command(StompFrame::commandSend)
                              || frame->is_command(StompFrame::commandNack))
                              && frame->is_header("transaction")
                              && !frame->execute_transaction();
    if (!is_intercept_trans) return false;

    std::string transaction_id = frame->get_header("transaction");
    Transaction *trans = peer->find_transaction(transaction_id);
    if (trans == NULL) {
      peer->send_error("transaction not found");

      // we don't want to continue processing
      // after this function returns so act like
      // it was a transaction even though it was an error
      return true;
    } // if

    // this was a transaction, store it for later
    trans->store(frame);
    return true;
  } // StompServer::_store_transaction

  void StompServer::_process_send(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("destination")) {
      peer->send_error("send missing destination");
      return;
    } // if

    openframe::StringToken st;
    st.setDelimiter('/');
    std::string destination = frame->get_header("destination");
    st = destination;

    if (st.size() < 2) {
      peer->send_error("destination invalid");
      return;
    } // if

    std::string key = st.trail(0);

    if (st[0] == "topic") {
      Exchange *exch = _exch_manager->create_exchange(key, Exchange::exchangeTypeTopic);
      StompMessage *smesg = new StompMessage( st.trail(0), frame->body());
      smesg->copy_headers_from(frame);	 // copy headers
//smesg->dont_delete();
      exch->post(smesg);	// post retains
      smesg->release();
    } // if
    else if (st[0] == "queue") {
      Exchange *exch = _exch_manager->create_exchange(key, Exchange::exchangeTypeFanout);
      exch->set_byte_limit(_queue_byte_limit);
      StompMessage *smesg = new StompMessage( st.trail(0), frame->body(), _time_queue_expire);
      smesg->copy_headers_from(frame);	// copy headers
//smesg->dont_delete();
      exch->post(smesg);	// post retains
      smesg->release();
    } // else if
    else {
      peer->send_error("destination must be a topic or queue");
      return;
    } // else
  } // StompServer::_process_send

  void StompServer::_process_subscribe(StompPeer *peer, StompFrame *frame) {
    stompHeader_t headers;
    if (!frame->is_header("destination")) {
      peer->send_error("subscribe missing destination");
      return;
    } // if

    if (!frame->is_header("id")) {
      peer->send_error("required id field missing");
      return;
    } // if

    Subscription::ackModeEnum ack = Subscription::ackModeAuto;
    if (frame->is_header("ack")) {
      std::string ack = frame->get_header("ack");
      if (ack == "client")
        ack = Subscription::ackModeClient;
      else if (ack == "client-individual")
        ack = Subscription::ackModeClientIndv;
      else if (ack == "auto")
        ack = Subscription::ackModeAuto;
      else {
        peer->send_error("ack must be auto or client");
        return;
      } // else
    } // if

    int prefetch = 0;
    if (frame->is_header("openstomp.prefetch")) {
      prefetch = atoi( frame->get_header("openstomp.prefetch").c_str() );
    } // if

    openframe::StringToken st;
    st.setDelimiter('/');
    std::string destination = frame->get_header("destination");
    st = destination;

    if (st.size() < 2) {
      peer->send_error("destination invalid");
      return;
    } // if

    std::string id = frame->get_header("id");
    if (id.length() < 1) {
      peer->send_error("invalid subscription id");
      return;
    } // if

    Subscription *sub = _exch_manager->find_subscription(peer, id);
    if (sub != NULL) {
      peer->send_error("subscription id already in use");
      return;
    } // if

    if (st[0] == "topic") {
      Subscription *sub = new Subscription(peer, id, st.trail(0), ack);
      sub->elogger( elogger(), elog_name() );
      if (prefetch) sub->prefetch(prefetch);
      _exch_manager->subscribe(sub);
      sub->release();
      //peer->subscribe(peer, id, st.trail(0), ack);
    } // if
    else if (st[0] == "queue") {
      Subscription *sub = new Subscription(peer, id, st.trail(0), ack);
      sub->elogger( elogger(), elog_name() );
      if (prefetch) sub->prefetch(prefetch);
      _exch_manager->subscribe(sub);
      sub->release();
    } // else if
    else {
      peer->send_error("destination must be a topic or queue");
      return;
    } // else
  } // StompServer::_process_subscribe

  void StompServer::_process_unsubscribe(StompPeer *peer, StompFrame *frame) {
    stompHeader_t headers;

    if (!frame->is_header("id")) {
      peer->send_error("unsubscribe missing session id");
      return;
    } // if
    std::string session_id = frame->get_header("id");

    Subscription *sub = peer->find_subscription(session_id);
    if (sub == NULL) {
      peer->send_error("subscription not found");
      return;
    } // if

    _exch_manager->unsubscribe(sub);
  } // StompServer::_process_unsubscribe

  void StompServer::_process_receipt_id(StompPeer *peer, StompFrame *frame) {
    if (!frame->is_header("receipt-id")) return;

    std::string receipt_id = frame->get_header("receipt-id");
    peer->receipt(receipt_id);
  } // StompServer::_process_receipt_id

  void StompServer::_process_disconnect(StompPeer *peer, StompFrame *frame) {
    peer->wantDisconnect();
  } // StompServer::_process_disconnect

  void StompServer::onConnect(const openframe::Connection *con) {
    StompPeer *peer;

    LOG(LogNotice, << "Connected to stomp peer " << derive_peer_ip(con->sock) << ":" << derive_peer_port(con->sock) << std::endl);

    try {
      peer = new StompPeer(con->sock);
      peer->intval_logstats(_intval_logstats);
      peer->set_elogger(elogger(), elog_name() );
      peer->replace_stats( stats() );
    } // try
    catch(std::bad_alloc xa) {
      assert(false);
    } // catch

    openframe::scoped_lock slock(&_peers_l);
    _peers.insert( std::make_pair(con->sock, peer) );
    ++_stats.num_peers;

    return;
  } // Worker::onConnect

  // ### Queue Members ###
  void StompServer::enqueue_queue(StompMessage *message) {
    _queues.insert(message);
    LOG(LogInfo, << "+   queueQ " << message->toString() << std::endl);
  } // StompServer::enqueue_queue
  void StompServer::enqueue_queue(mesgList_t &unsent) {
    while( !unsent.empty() ) {
      StompMessage *smesg = unsent.front();
      enqueue_queue(smesg);
      unsent.pop_front();
    } // while
  } // StompServer::enqueue_queue
  void StompServer::exqueue_queues() {
    std::deque<queues_itr> rm;
    for(queues_itr itr = _queues.begin(); itr != _queues.end(); itr++) {
      if ((*itr)->is_inactive())
        rm.push_back(itr);
    } // for

    while(!rm.empty()) {
      StompMessage *smesg = *rm.front();
      LOG(LogInfo, << "E   queueQ " << smesg->toString() << std::endl);
      smesg->release();
      _queues.erase(rm.front());
      rm.pop_front();
    } // while
  } // StompServer::exqueue_queues
  void StompServer::clear_queues() {
    std::deque<queues_itr> rm;
    for(queues_itr itr = _queues.begin(); itr != _queues.end(); itr++) {
      StompMessage *smesg = (*itr);
      smesg->release();
    } // for

    _queues.clear();
  } // StompServer::clear_queues
  void StompServer::dequeue_queue(StompMessage *message) {
    LOG(LogInfo, << "-   queueQ " << message->toString() << std::endl);
  } // StompServer::dequeue_queue

  void StompServer::enqueue_topic(StompMessage *message) {
    _topics.insert(message);
    LOG(LogInfo, << "+   topicQ " << message->toString() << std::endl);
  } // StompServer::enqueue_topic
  void StompServer::exqueue_topics() {
    std::deque<topics_itr> rm;
    for(topics_itr itr = _topics.begin(); itr != _topics.end(); itr++) {
      if ((*itr)->is_inactive())
        rm.push_back(itr);
    } // for

    while(!rm.empty()) {
      StompMessage *smesg = *rm.front();
      LOG(LogInfo, << "E   topicQ " << smesg->toString() << std::endl);
      smesg->release();
      _topics.erase(rm.front());
      rm.pop_front();
    } // while
  } // StompServer::exqueue_topics
  void StompServer::dequeue_topic(StompMessage *message) {
    LOG(LogInfo, << "-   topicQ " << message->toString() << std::endl);
  } // StompServer::dequeue_topic

  void StompServer::onDisconnect(const openframe::Connection *con) {
    peers_itr itr;

    openframe::scoped_lock slock(&_peers_l);
    itr = _peers.find(con->sock);

    // not found, uhoh
    if (itr == _peers.end()) assert(false);	// bug

    StompPeer *peer = itr->second;

    _process_peer(peer);
    --_stats.num_peers;
    LOG(LogNotice, << "Disconnected from stomp peer " << peer << std::endl);

    // recover any messages enqueue
    _exch_manager->unsubscribe(peer);

    peer->wantDisconnect();

    _peers.erase(itr);
    peer->release();
    return;
  } // StompServer::onDisconnect

  void StompServer::onRead(const openframe::Peer *lis) {
    openframe::scoped_lock slock(&_peers_l);

    peers_itr itr = _peers.find(lis->sock);
    if (itr == _peers.end()) return;

    StompPeer *peer = itr->second;
    if (_debug) {
      stringstream out;
//      out << "[ STOMP Packet from " << lis->peer_str << " " << std::setprecision(3) << std::fixed << peer->pps() << " pps ]";
      LOG(LogDebug, << "<" << openframe::StringTool::ppad(out.str(), "-", 79) << std::endl);
      LOG(LogDebug, << openframe::StringTool::hexdump(lis->in, "<   ") << "<" << std::endl);
    } // if

    peer->receive(lis->in, lis->in_len);

    return;
  } // StompServer::onRead

  const string::size_type StompServer::onWrite(const openframe::Peer *lis, std::string &ret) {
    openframe::scoped_lock slock(&_peers_l);
    peers_itr ptr = _peers.find(lis->sock);
    if (ptr != _peers.end()) ptr->second->transmit(ret);

    return ret.size();
  } // StompServer::onWrite

  bool StompServer::onPeerWake(const openframe::Peer *lis) {
    openframe::scoped_lock slock(&_peers_l);
    peers_itr itr = _peers.find(lis->sock);
    if (itr == _peers.end()) return false;
    StompPeer *peer = itr->second;

    if (!peer->disconnect()) return false;

    safe_disconnect(lis->sock);

    return true;
  } // StompServer::onPeerWake
} // namespace stomp
