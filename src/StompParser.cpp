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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <openframe/openframe.h>
#include <openstats/openstats.h>

#include "StompMessage.h"
#include "StompParser.h"
#include "Subscription.h"
#include "Transaction.h"

#undef STOMP_PARSER_DEBUG

namespace stomp {
  using namespace openframe;
  using namespace openframe::loglevel;
  using namespace std;

/**************************************************************************
 ** StompPeer Class                                                       **
 **************************************************************************/

  const time_t StompParser::kDefaultStatsInterval		= 300;

  StompParser::StompParser() {
    _init();
    return;
  } // StompParser::StompParser

  StompParser::~StompParser() {
    _clear_frameq();
    _process_reset();
    unbind_all();
    onDestroyStats();
    return;
  } // StompParser::~StompParser

  void StompParser::_init() {
    // timers
    memset(&_heart_beat, 0, sizeof(heart_beat_t));
    init_stats(true);
    reset();
    _process_reset();
  } // StompPeer::_init

  void StompParser::init_stats(const bool startup) {
    _stats.num_frames_in = 0;
    _stats.num_frames_out = 0;
    _stats.num_bytes_in = 0;
    _stats.num_bytes_out = 0;
    _stats.last_report_at = time(NULL);
    if (startup) _stats.created_at = time(NULL);
  } // StompParser::init_stats

  bool StompParser::try_stats() {
    if (_stats.last_report_at > time(NULL) - kDefaultStatsInterval) {
      return false;
    } // if
    init_stats();
    return true;
  } // StompParser::try_stats

  void StompParser::onDescribeStats() {
    set_stat_id_prefix("libstomp.stomppeer."+_uniq_id);
    describe_stat("num.frames.in", "num frames in", openstats::graphTypeGauge, openstats::dataTypeInt, openstats::useTypeSum);
    describe_stat("num.frames.out", "num frames out", openstats::graphTypeGauge, openstats::dataTypeInt, openstats::useTypeSum);
    describe_stat("num.bytes.in", "num bytes in", openstats::graphTypeGauge, openstats::dataTypeInt, openstats::useTypeSum);
    describe_stat("num.bytes.out", "num bytes in", openstats::graphTypeGauge, openstats::dataTypeInt, openstats::useTypeSum);
  } // StompParser::onDescribeStats

  void StompParser::onDestroyStats() {
    stats()->destroy_stat("libstomp.stomppeer."+_uniq_id+"*");
  } // StompParser::onDestroyStats

  void StompParser::enable_heart_beat(const time_t sender_supported, const time_t sender_wants) {
    _heart_beat.enabled = true;
    _heart_beat.ping_out = (sender_wants / 1000);
    _heart_beat.ping_in = (sender_supported / 1000);

    _heart_beat.last_ping_out = openframe::Stopwatch::Now();
    _heart_beat.last_ping_in = openframe::Stopwatch::Now();
  } // StompParser::enable:heart_beat

  void StompParser::try_heart_beat() {
    if (!_heart_beat.enabled) return;

    bool ok = is_heart_beat_time();
    if (ok) _write("\n");
  } // StompParser::try_heart_beat

  bool StompParser::is_heart_beat_time() {
    double now = openframe::Stopwatch::Now();
    return _heart_beat.enabled
           && _heart_beat.ping_out
           && (_heart_beat.last_ping_out < now - _heart_beat.ping_out);
  } // StompParser::is_heart_beat_time

  bool StompParser::is_heart_beat_timeout() {
    double now = openframe::Stopwatch::Now();
    return _heart_beat.enabled
           && _heart_beat.ping_in
           && (_heart_beat.last_ping_in < now - (_heart_beat.ping_in * 3));
  } // StompParser::is_heart_beat_timeout

  const size_t StompParser::send_error(const std::string &message, const std::string &body) {
    StompFrame *frame = new StompFrame("ERROR", body);
    frame->add_header("message", message);
    bool ok = send_frame(frame);
    onRecoverableError(frame);
    frame->release();
    return ok;
  } // StompParser::send_error

  const size_t StompParser::disconnect_with_error(const std::string &message, const std::string &body) {
    StompFrame *frame = new StompFrame("ERROR", body);
    frame->add_header("message", message);
    bool ok = send_frame(frame);
    onFatalError(frame);
    frame->release();
    return ok;
  } // StompParser::disconnect_with_error

  bool StompParser::commit_transaction(const std::string &key) {
    Transaction *trans = find_transaction(key);
    if (trans == NULL) return false;

    Transaction::queue_t queue;
    trans->dequeue(queue);

    while( !queue.empty() ) {
      StompFrame *frame = queue.front();
      _frameQ.push(frame);
      queue.pop_front();
    } // while

    destroy_transaction(key);
    return true;
  } // StompParser::commit_transaction

  const size_t StompParser::send_frame(StompFrame *frame) {
    assert(frame != NULL); // bug
    std::string ret = frame->compile();
    size_t len = ret.length();

    _stats.num_frames_out++;
    _stats.num_bytes_out += len;
    datapoint("num.frames.out", 1);
    datapoint("num.bytes.out", len );
    return _write(ret);
  } // StompParser::send_frame

  size_t StompParser::receive(const char *buf, const size_t len) {
    openframe::scoped_lock slock(&_in_l);
    _heart_beat.last_ping_in = openframe::Stopwatch::Now();
    _in.append(buf, len);
    _stats.num_bytes_in += len;

    datapoint("num.bytes.in", len);
    return len;
  } // StompParser::receive

  bool StompParser::next_frame(StompFrame *&frame) {
    for(size_t i=0; i < 100 && process(); i++);

    if ( _frameQ.empty() ) return false;
    frame = _frameQ.front();
    _frameQ.pop();
    return true;
  } // next_frame

  bool StompParser::process() {
    for(binds_itr itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      StompMessage *smesg;

      size_t limit = 1000;
      for(size_t i = 0; i < limit && sub->dequeue_for_send(smesg); i++) {
        smesg->replace_header("subscription", sub->id() );
        send_frame( dynamic_cast<StompFrame *>(smesg) );
        if ( !smesg->requires_resp() ) smesg->release(); // if we dont need response release
      } // for
    } // for

    try_heart_beat();

    openframe::scoped_lock slock(&_in_l);
    if (_in.length() < 1) return false;

    bool ret;

    switch(_stage) {
      case stompStageCommand:
        ret = _process_command();
        break;
      case stompStageHeaders:
        ret = _process_headers();
        break;
      case stompStageBodyLen:
        ret = _process_body_len();
        break;
      case stompStageBodyNul:
        ret = _process_body_nul();
        break;
      case stompStageWaitForNul:
        ret = _process_waitfor_nul();
        break;
      default:
        assert(false);		// bug
        break;
    } // switch

#ifdef STOMP_PARSER_DEBUG
std::cout << _stagedFrame.command << " stage(" << _stage << ") ret(" << ret << ")" << std::endl;
#endif

    return ret;
  } // StompParer::process

  bool StompParser::_process_command() {
    string command;
    command.reserve(30);

    bool ok = _in.sfind('\n', command);
    if (!ok) return false;

#ifdef STOMP_PARSER_DEBUG
std::cout << "COMMAND(" << command << ")" << std::endl;
#endif

    StringTool::replace("\r", "", command);

    if (command.length() == 0) {
      _process_reset();
      return true;		// in case this is the end of a frame
    } // if

    _stagedFrame.command = command;
    _stage = stompStageHeaders;
    return true;
  } // StompParser::process_command

  bool StompParser::_process_headers() {
    StompHeader *header;
    string header_str;

    size_t num_headers = 0;
    while(1) {
//std::cout << "MEH(" << _in.substr(_in_pos) << ")" << std::endl;
      bool ok = _in.sfind('\n', header_str);
      if (!ok) return (num_headers ? true : false);

      num_headers++;

#ifdef STOMP_PARSER_DEBUG
std::cout << "LEN(" << len << ") HEADER(" << header_str << ")" << std::endl;
#endif

//      ret += header_str.length();
      StringTool::replace("\r", "", header_str);
      if (header_str.length() == 0) {
        stompHeader_itr itr = _stagedFrame.headers.find("Content-Length");
        if (itr != _stagedFrame.headers.end()) {
          _stagedFrame.content_length = atoi( itr->second->value().c_str() );
          _stage = stompStageBodyLen;
        } // if
        else
          _stage = stompStageBodyNul;
        break;
      } // if

      try {
        header = new StompHeader(header_str);
      } // try
      catch(Stomp_Exception ex) {
        // unable to parse header send error
        send_error("unable to parse header");
        _process_reset();
        break;
      } //catch

      if (_stagedFrame.headers.find(header->name()) != _stagedFrame.headers.end()) {
        // stomp 1.1 spec says only the first header of the same name is accepted
        header->release();
        continue;
      } // if

      _stagedFrame.headers.insert( make_pair(header->name(), header) );

    } // while

    return true;
  } // StompParser::_process_headers

  bool StompParser::_process_body_len() {
    size_t ret = 0;

    bool ok = _in.next_bytes(_stagedFrame.content_length, _stagedFrame.body);
    if (!ok) return false;

    ret = _stagedFrame.body.length();
    _stage = stompStageWaitForNul;

    return true;
  } // StompParser::_process_body_len

  bool StompParser::_process_waitfor_nul() {
    std::string ret;
    bool ok = _in.sfind('\0', ret);
    if (!ok) return false;

    StompFrame *frame = new StompFrame(_stagedFrame.command, _stagedFrame.body);
    frame->insert(_stagedFrame.headers.begin(), _stagedFrame.headers.end());
    _stagedFrame.headers.clear();

    _frameQ.push(frame);
    _stats.num_frames_in++;
    datapoint("num.frames.in", 1);
    _process_reset();

    return true;
  } // StompParser::_process_waitfor_nul

  bool StompParser::_process_body_nul() {
    bool ok = _in.sfind('\0', _stagedFrame.body);
    if (!ok) return false;
//std::cout << "LEN(" << len << ") POS(" << _in_pos << ") _IN(" << _in.length() << ")" << std::endl;

#ifdef STOMP_PARSER_DEBUG
std::cout << "BODY(" << _stagedFrame.body << ")" << std::endl;
#endif

    StompFrame *frame = new StompFrame(_stagedFrame.command, _stagedFrame.body);
    frame->insert(_stagedFrame.headers.begin(), _stagedFrame.headers.end());
    _stagedFrame.headers.clear();

    _frameQ.push(frame);

    _stats.num_frames_in++;
    datapoint("num.frames.in", 1);
    _process_reset();

    return true;
  } // StompParser::_process_body_nul

  void StompParser::_process_reset() {
    _stagedFrame.command = "";
    _stagedFrame.body = "";
    _stagedFrame.content_length = 0;

    for(stompHeader_itr itr = _stagedFrame.headers.begin(); itr != _stagedFrame.headers.end(); itr++)
      itr->second->release();

    _stagedFrame.headers.clear();
    _stage = stompStageCommand;
  } // StompParser::_process_reset

  const string::size_type StompParser::_write(const string &buf) {
    openframe::scoped_lock slock(&_out_l);
    _heart_beat.last_ping_out = openframe::Stopwatch::Now();
    _out.append(buf);
    return buf.size();
  } // StompParser::_write

  const string::size_type StompParser::transmit(string &ret) {
    openframe::scoped_lock slock(&_out_l);
    ret = _out;
    _out = "";
    return ret.size();
  } // StompParser::transmit

  void StompParser::reset() {
    _stage = stompStageCommand;

    init_stats(true);

    openframe::scoped_lock sl_out(&_out_l);
    openframe::scoped_lock sl_in(&_in_l);
    _in = _out = "";
    _process_reset();
  } // StompParser::reset

  /**************************************************************************
   ** Commands                                                             **
   **************************************************************************/
  bool StompParser::connect(const std::string &username, const std::string &passcode, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("CONNECT");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    if (username.length()) {
      frame->replace_header("login", username);
      frame->replace_header("passcode", passcode);
    } // if

    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::connect

  bool StompParser::connected(const string &session_id, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("CONNECTED");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("session", session_id);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::connected

  bool StompParser::send(const std::string &destination, const std::string &body, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("SEND", body);
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("destination", destination);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::send

  bool StompParser::subscribe(const std::string &destination, const std::string &subscription,
                              StompHeaders *headers) {
    StompFrame *frame = new StompFrame("SUBSCRIBE");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("destination", destination);
    frame->replace_header("id", subscription);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::subscribe

  bool StompParser::unsubscribe(const string &destination, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("UNSUBSCRIBE");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("destination", destination);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::unsubscribe

  bool StompParser::begin(const std::string &transaction, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("BEGIN");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("transaction", transaction);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::begin

  bool StompParser::commit(const std::string &transaction, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("COMMIT");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("transaction", transaction);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::commit

  bool StompParser::ack(const std::string &message_id, const std::string &subscription, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("ACK");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("message-id", message_id);
    frame->replace_header("subscription", subscription);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::ack

  bool StompParser::nack(const std::string &message_id, const std::string &subscription, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("NACK");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("message-id", message_id);
    frame->replace_header("subscription", subscription);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::nack

  bool StompParser::abort(const string &transaction, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("ABORT");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("transaction", transaction);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParer::abort

  bool StompParser::message(const std::string &destination, const std::string &message_id,
                            const std::string &body, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("MESSAGE", body);
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("destination", destination)
          .replace_header("message-id", message_id);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::message

  bool StompParser::receipt(const std::string &receipt_id, StompHeaders *headers) {
    StompFrame *frame = new StompFrame("RECEIPT");
    if (headers) {
      frame->copy_headers_from(headers);
      headers->release();
    } // if

    frame->replace_header("receipt-id", receipt_id);
    size_t ret = send_frame(frame);
    frame->release();

    return ret > 0 ? true : false;
  } // StompParser::receipt

  // ### Subscriptions ###
  bool StompParser::bind(Subscription *sub) {
    assert(sub != NULL);
    binds_itr itr = _binds.find(sub);
    if (itr != _binds.end()) return false;
    sub->retain();
    _binds.insert(sub);
    //log(LogInfo) << "StompParser bound to " << sub << endl;
    onBind(sub);
    return true;
  } // StompParser::bind

  bool StompParser::unbind(Subscription *sub) {
    assert(sub != NULL);
    binds_itr itr = _binds.find(sub);
    if (itr == _binds.end()) return false;
    //log(LogInfo) << "StompParser unbound from " << sub << endl;
    onUnbind(sub);
    _binds.erase(sub);
    sub->release();
    return true;
  } // StompParser::unbind

  void StompParser::unbind_all() {
    for(binds_itr itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      onUnbind(sub);
      //log(LogInfo) << "StompParser unbound from " << sub << endl;
      sub->release();
    } // for
    _binds.clear();
  } // StompParser::unbind_all

  bool StompParser::is_bound(const string &id) {
    Subscription *sub = find_bind(id);
    return sub == NULL ? false : true;
  } // is_bound

  Subscription *StompParser::find_bind(const std::string &id) {
    for(binds_itr itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      if ( sub->is_id(id) ) return sub;
    } // for

    return NULL;
  } // StompParser::find_bind

  const bool StompParser::is_match(const string &name, Subscription *&ret) {
    binds_itr itr;
    for(itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      if ( sub->match(name) && sub->prefetch_ok() ) {
        ret = sub;
        return true;
      } // if
    } // for

    return false;
  } // StompParser::is_match

  bool StompParser::store_subscription(Subscription *sub) {
    subscriptions_itr itr = _subscriptions.find( sub->id() );
    if (itr != _subscriptions.end()) return false;
    _subscriptions[ sub->id() ] = sub;
    return false;
  } // StompParser::store_subscription

  bool StompParser::forget_subscription(Subscription *sub) {
    subscriptions_itr itr = _subscriptions.find( sub->id() );
    if (itr == _subscriptions.end()) return false;
    _subscriptions.erase(itr);
    return true;
  } // StompParser::forget_subscription

  Subscription *StompParser::find_subscription(const std::string &id) {
    subscriptions_itr itr = _subscriptions.find(id);
    if (itr == _subscriptions.end()) return NULL;
    return _subscriptions[id];
  } // StompParser::find_subscription

  const bool StompParser::received_ack(const string &id, const string &message_id) {
    for(binds_itr itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      if ( sub->is_id(id) ) return sub->dequeue(message_id);
    } // for
    return false;
  } // StompParser::received_ack

  const bool StompParser::received_nack(const string &id, const string &message_id) {
    for(binds_itr itr = _binds.begin(); itr != _binds.end(); itr++) {
      Subscription *sub = *itr;
      if ( sub->is_id(id) ) return sub->redeliver(message_id);
    } // for
    return false;
  } // StompParser::received_nack

  void StompParser::_clear_frameq() {
    while( !_frameQ.empty() ) {
      StompFrame *frame = _frameQ.front();
      frame->release();
      _frameQ.pop();
    } // while
  } // StompParser::_clear_frameq

  std::ostream &operator<<(std::ostream &ss, const StompParser::stompStageEnum stage) {
    switch(stage) {
      case StompParser::stompStageCommand:
        ss << "stageCommand";
        break;
      case StompParser::stompStageHeaders:
        ss << "stageHeaders";
        break;
      case StompParser::stompStageBodyLen:
        ss << "stageBodyLen";
        break;
      case StompParser::stompStageBodyNul:
        ss << "stageBodyNul";
        break;
      case StompParser::stompStageWaitForNul:
        ss << "stageWaitForNul";
        break;
      default:
        ss << "unknown";
        break;
    } // switch
    return ss;
  } // operator<<
} // namespace stomp

