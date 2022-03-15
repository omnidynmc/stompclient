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
#include "Exchange_Topic.h"
#include "Subscription.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** Exchange Class                                                       **
 **************************************************************************/
  Exchange_Topic::Exchange_Topic(const std::string &key) :
    Exchange(key) {
    _type = exchangeTypeTopic;
  } // Exchange_Topic::Exchange_Topic

  Exchange_Topic::~Exchange_Topic() {
  } // Exchange_Topic::~Exchange_Topic

  const string Exchange_Topic::toString() const {
    std::stringstream out;
    out << "Exchange_Topic "
        << "key=" << key()
        << ",binds=" << _binds.size()
        << ",sendq=" << _sendq.size()
        << ",unackd=" << _unackd.size();
    return out.str();
  } // Exchange_Topic::toString

  size_t Exchange_Topic::onDispatch(const size_t limit) {
    size_t num;
    size_t num_deferred = 0;
    size_t num_dispatched = 0;

    bool is_work_pending = !_sendq.empty();
    if (!is_work_pending) return 0;

    for(num=0; num < limit && _sendq.size(); num++) {
      StompMessage *smesg = _sendq.front();
      _sendq.pop_front();

      list_t subs;
      bool found = find_matches(smesg->destination(), subs);
      if (!found) {
        // no matching subs so we drop this packet
        dispatched(smesg);
        num_deferred++;
        continue;
      } // if

      // pass smesg off to Subcription and release our interest for now
      for(list_st i=0; i < subs.size(); i++) {
        smesg->requires_resp(false);
        subs[i]->enqueue(smesg);
        LOG(LogDebug, << "Exchange_Topic delivering message; " << smesg
                      << " to subscription " << (i+1) << "; " << subs[i] << std::endl);
      } // for
      num_dispatched++;
      dispatched(smesg);
    } // for

    _stats.num_dispatched += num_dispatched;
    _stats.num_deferred += num_deferred;
    _stats.num_sendq -= num;

    datapoint("num.dispatched", num);
    datapoint("num.deferred", num_deferred);

    return num_dispatched;
  } // Exchange_Topic::dispatch

//  std::ostream &operator<<(std::ostream &ss, const Exchange *exch) {
//    ss << exch->toString();
//    return ss;
//  } // operator<<
} // namespace stomp
