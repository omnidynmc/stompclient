#include "config.h"

#include <string>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <openframe/openframe.h>

#include "StompPeer.h"
#include "StompMessage.h"
#include "Subscription.h"

namespace stomp {
  using namespace openframe::loglevel;
  using openframe::StringTool;
  using std::string;
  using std::stringstream;
  using std::cout;
  using std::endl;

/**************************************************************************
 ** Subscription Class                                                   **
 **************************************************************************/
  const time_t Subscription::kDefaultStatsInterval	= 5;

  Subscription::Subscription(StompPeer *peer, const string &id, const string &key, const ackModeEnum ack_mode) :
      _peer(peer), _id(id), _key(key), _ack_mode(ack_mode), _prefetch(0) {
     assert(peer != NULL);
    _peer->retain();
    _peer->store_subscription(this);

    init_stats(kDefaultStatsInterval, true);

    _profile = new openframe::Stopwatch();
    _profile->add("dequeue", 300);
    _profile->add("redelivered", 300);
  } // Subscription::Subscription

  Subscription::~Subscription() {
    _peer->forget_subscription(this);
    _peer->release();
    delete _profile;
  } // Subscription::~Subscription

  const bool Subscription::match(const string &key) const {
    return StringTool::match(_key.c_str(), key.c_str());
  } // match

  void Subscription::init_stats(const time_t report_interval, const bool startup) {
    _stats.num_enqueued = 0;
    _stats.num_dequeued = 0;
    _stats.num_redelivered = 0;
    _stats.last_stats_at = time(NULL);
    if (report_interval) _stats.report_interval = report_interval;
    if (startup) _stats.created_at = time(NULL);
  } // Subscription::init_stats

  bool Subscription::try_stats() {
    if (_stats.last_stats_at > time(NULL) - _stats.report_interval) return false;

    if (_stats.num_dequeued) {
        LOG(LogInfo, << "Subscription dequeued " << _stats.num_dequeued
                     << " to " << this << std::endl);
    } // if

    if (_stats.num_redelivered) {
        LOG(LogInfo, << "Subscription nack'ed " << _stats.num_redelivered
                     << " messages to " << this << std::endl);
    } // if

    init_stats();
    return true;
  } // Subscription::try_stats

  void Subscription::enqueue(StompMessage *smesg) {
    smesg->retain();
    _sendq.push_back( smesg );
    _stats.num_enqueued++;
  } // Subscription::enqueue

  void Subscription::bind() {
    _peer->bind(this);
  } // Subscription::bind

  void Subscription::unbind() {
    _peer->unbind(this);
  } // Subscription::unbind

  const bool Subscription::dequeue_for_send(StompMessage *&smesg) {
    bool is_work_pending = !_sendq.empty();
    if (!is_work_pending) return false;
    smesg = _sendq.front();
    smesg->inc_attempt();
    _sendq.pop_front();
    if ( smesg->requires_resp() ) _sentq.push_back(smesg);
    return true;
  } // Subscription::dequeue_for_send

  size_t Subscription::dequeue(const string &id) {
    size_t num=0;
    bool found = false;

    openframe::Stopwatch sw;
    sw.Start();

    queue_itr first, last;
    first = _sentq.begin();
    for(queue_itr itr = _sentq.begin(); itr != _sentq.end(); itr++) {
      StompMessage *smesg = *itr;
      if ( smesg->is_id(id) ) {
        last = itr;
        last++;	// .erase is not inclusive of last
        found = true;
        break;
      } // if
    } // for

    _profile->average("dequeue", sw.Time());

    if (!found) return num;

    for(queue_itr itr = first; itr != last; itr++) {
      StompMessage *smesg = *itr;
      smesg->release();
      _stats.num_dequeued++;
      num++;
    } // for

//    LOG(LogInfo, << "Subscription dequeued " << num << "; " << this << std::endl);
    _sentq.erase(first, last);

    return num;
  } // Subscription::dequeue

  size_t Subscription::redeliver(const string &id) {
    size_t num=0;
    bool found = false;

    openframe::Stopwatch sw;
    sw.Start();

    queue_itr first, last;
    first = _sentq.begin();
    for(queue_itr itr = _sentq.begin(); itr != _sentq.end(); itr++) {
      StompMessage *smesg = *itr;
      if ( smesg->is_id(id) ) {
        last = itr;
        last++;	// .erase is not inclusive of last
        found = true;
        break;
      } // if
    } // for

    _profile->average("redelivered", sw.Time());

    if (!found) return num;

    for(queue_itr itr = first; itr != last; itr++) {
      StompMessage *smesg = *itr;
      _stats.num_redelivered++;
      num++;
    } // for

    _deadq.insert(_deadq.end(), first, last);
    _sentq.erase(first, last);

    return num;
  } // Subscription::redeliver

  mesgList_st Subscription::dequeue_dead(mesgList_t &ret, size_t limit) {
    mesgList_st num = 0;

    // we're passing these off so don't release them
    while( !_deadq.empty() && (num < limit || limit == 0)) {
      queue_itr itr = _deadq.begin();
      StompMessage *smesg = *itr;
      ret.push_back(smesg);
      _deadq.erase(itr);
      num++;
    } // while

    return num;
  } // Subscription::dequeue_dead

  void Subscription::dequeue_all(mesgList_t &ret) {
    // we're passing these off so don't release them
    while( !_sendq.empty() ) {
      queue_itr itr = _sendq.begin();
      StompMessage *smesg = *itr;
      ret.push_back(smesg);
      _sendq.erase(itr);
    } // while

    while( !_sentq.empty() ) {
      queue_itr itr = _sentq.begin();
      StompMessage *smesg = *itr;
      ret.push_back(smesg);
      _sentq.erase(itr);
    } // while

    dequeue_dead(ret);
  } // Subscription::dequeue_all

  const string Subscription::toString() const {
    stringstream out;
    out << "Subscription key=" << _key
        << ",sendq=" << sendq()
        << ",sentq=" << sentq()
        << ",id=" << _id
        << ",peer=" << _peer;
    return out.str();
  } // Subscription::toString

  const string Subscription::toStats() const {
    stringstream out;
    time_t diff = time(NULL) - _stats.created_at;
    out << "sendq=" << sendq()
        << ",sentq=" << sentq()
        << ",eq/s=" << std::fixed << std::setprecision(2) << float(_stats.num_enqueued) / float(diff)
        << ",dq/s=" << std::fixed << std::setprecision(2) << float(_stats.num_dequeued) / float(diff)
        << ",adq/s=" << std::fixed << std::setprecision(6) << _profile->average("dequeue")
        << ",prefetch=" << _prefetch
        << ",enqueued=" << std::setprecision(2) << std::fixed << float(_stats.num_enqueued)/1000 << "k"
        << ",dequeued=" << std::setprecision(2) << std::fixed << float(_stats.num_dequeued)/1000 << "k";
    return out.str();
  } // Subscription::toStats

  std::ostream &operator<<(std::ostream &ss, const Subscription *sub) {
    ss << sub->toString();
    return ss;
  } // operator<<
} // namespace stomp
