#include "config.h"

#include <string>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <openframe/openframe.h>

#include "StompMessage.h"
#include "Exchange_Fanout.h"
#include "Subscription.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** Exchange Class                                                       **
 **************************************************************************/
  Exchange_Fanout::Exchange_Fanout(const std::string &key) :
    Exchange(key), _index(0) {
    _type = exchangeTypeFanout;
  } // Exchange_Fanout::Exchange_Fanout

  Exchange_Fanout::~Exchange_Fanout() {
  } // Exchange_Fanout::~Exchange_Fanout

  const string Exchange_Fanout::toString() const {
    std::stringstream out;
    time_t diff = time(NULL) - _last_stats;
    double posts_ps = 0;
    if (diff) posts_ps = double(_num_posts) / double(diff);
    out << "Exchange_Fanout "
        << "key=" << key()
        << ",posts=" << std::fixed << std::setprecision(2) << posts_ps << "/s"
        << ",binds=" << _binds.size()
        << ",sendq=" << _sendq.size()
        << ",unackd=" << _unackd.size()
        << ",bytes=" << _num_bytes;
    return out.str();
  } // Exchange_Fanout::toString

  size_t Exchange_Fanout::onDispatch(const size_t limit) {
    size_t num;
    size_t num_dispatched = 0;
    size_t num_deferred = 0;

    if (_last_stats < time(NULL) - kDefaultStatsIntval) {
      _last_stats = time(NULL);
      _num_posts = 0;
    } // if

    bool is_work_pending = !_binds.empty() && !_sendq.empty() && is_next_dispatch();
    if (!is_work_pending) return 0;

    for(num=0; num < limit && !_sendq.empty(); num++) {
      StompMessage *smesg = _sendq.front();
      _sendq.pop_front();

      list_t subs;
      bool found = find_matches(smesg->destination(), subs);
      if (!found) {
//        log(LogDebug) << "Exchange_Fanout deferred message; " << smesg << std::endl;
        _deferd.push_front(smesg);
        num_deferred++;
        continue;
      } // if

      // pass smesg off to Subcription and release our interest for now
      unsigned int index = (_index++ % subs.size());
      subs[index]->enqueue(smesg);
//      log(LogDebug) << "Exchange_Fanout delivering message; " << smesg
//                    << " to index " << (index+1) << "; " << subs[index] << std::endl;
      num_dispatched++;
      dispatched(smesg);
    } // for

    if ( !_deferd.empty() ) {
      set_delayed_dispatch();
      // push deferd back onto the front of the queue
      while( !_deferd.empty() ) {
        StompMessage *smesg = _deferd.front();
        _sendq.push_front(smesg);
        _deferd.pop_front();
      } // while
    } // if

    _stats.num_sendq -= num_dispatched;
    _stats.num_dispatched += num_dispatched;
    _stats.num_deferred += num_deferred;

    datapoint("num.dispatched", num_dispatched);
    datapoint("num.deferred", num_deferred);

    return num;
  } // Exchange_Fanout::dispatch

//  std::ostream &operator<<(std::ostream &ss, const Exchange *exch) {
//    ss << exch->toString();
//    return ss;
//  } // operator<<
} // namespace stomp
