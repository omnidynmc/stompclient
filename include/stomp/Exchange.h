#ifndef __LIBSTOMP_EXCHANGE_H
#define __LIBSTOMP_EXCHANGE_H


#include <set>
#include <deque>
#include <string>

#include <openframe/openframe.h>
#include <openstats/StatsClient_Interface.h>

#include "StompMessage.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class Subscription;
  class StompPeer;
  class Exchange : public openframe::OpenFrame_Abstract,
                   public openframe::Refcount,
                   public openstats::StatsClient_Interface {
    public:
      typedef std::set<Subscription *> bind_t;
      typedef bind_t::iterator bind_itr;
      typedef bind_t::const_iterator bind_citr;
      typedef bind_t::size_type bind_st;

      typedef std::deque<StompMessage *> queue_t;
      typedef queue_t::size_type queue_st;

      typedef std::vector<Subscription *> list_t;
      typedef list_t::size_type list_st;

      enum exchangeTypeEnum {
        exchangeTypeDirect	= 0,
        exchangeTypeTopic	= 1,
        exchangeTypeFanout	= 2,
        exchangeTypeHeaders	= 3
      }; // exchangeTypeEnum

      static const time_t kDefaultExpireInterval;
      static const time_t kDefaultRecoverDeadInterval;
      static const time_t kDefaultDeferredInterval;
      static const time_t kDefaultExchangeStatsInterval;
      static const time_t kDefaultStatsIntval;
      static const size_t kDefaultByteLimit;
      static const size_t kDefaultExpireLimit;

      Exchange(const std::string &key);
      virtual ~Exchange();
      void onDescribeStats();
      void onDestroyStats();

      // pure virtuals
      virtual size_t onDispatch(const size_t limit) = 0;

      void post(StompMessage *smesg);

      bool bind(Subscription *sub);
      bool unbind(Subscription *sub);
      bind_st unbind(const string &id);
      bind_st unbind(StompPeer *peer);
      bind_st unbind_all();
      void recover_unsent(Subscription *sub);
      mesgList_st recover_dead();

      Exchange &expire_interval(const time_t ex) {
        _expire_interval = ex;
        return *this;
      } // expire_interval
      inline bool is_next_expire() const { return _last_expire < time(NULL) - _expire_interval; }
      inline bool is_over_byte_limit() const { return _num_bytes > _byte_limit; }

      inline bool is_next_recover_dead() const { return _last_recover_dead < time(NULL) - _recover_dead_interval; }

      Exchange &set_byte_limit(const time_t bl) {
        _byte_limit = bl;
        return *this;
      } // set_byte_limit

      Exchange &set_expire_limit(const time_t ex) {
        _expire_limit = ex;
        return *this;
      } // set_expire_limit

      Exchange &set_deferred_interval(const time_t ex) {
        _deferred_interval = double(ex / 1e6);
        return *this;
      } // set_deferred_delay
      inline time_t deferred_interval() const { return _deferred_interval; }
      inline void set_delayed_dispatch() { _last_dispatch = openframe::Stopwatch::Now(); }
      inline time_t is_next_dispatch() const { return _last_dispatch < openframe::Stopwatch::Now() - _deferred_interval; }

      virtual const std::string toString() const;
      virtual size_t dispatch(const size_t limit);
      virtual size_t expire_inactive(const size_t limit=0);
      const std::string key() const { return _key; }
      const exchangeTypeEnum type() const { return _type; }

      bind_st bind_size() const { return _binds.size(); }
      queue_st sendq_size() const { return _sendq.size(); }
      queue_st unackd_size() const { return _unackd.size(); }
      queue_st deferd_size() const { return _deferd.size(); }
      bool try_stats();

    protected:
      list_st find_matches(const string &key, list_t &ret);
      size_t inc_bytes(StompMessage *smesg);
      size_t dec_bytes(StompMessage *smesg);
      void dispatched(StompMessage *smesg);
      void expire(StompMessage *smesg);
      void init_stats(const time_t report_interval=0, const bool startup=false);

      exchangeTypeEnum _type;

      bind_t _binds;
      queue_t _sendq;
      queue_t _deferd;
      queue_t _unackd;

      size_t _num_posts;
      size_t _num_bytes;
      time_t _last_stats;

      struct obj_stats_t {
        size_t num_dispatched;
        size_t num_deferred;
        size_t num_posted;
        size_t num_sendq;
        size_t num_dead;
        time_t report_interval;
        time_t last_stats_at;
        time_t created_at;
      } _stats; // obj_stats_t

    private:

      std::string _key;
      time_t _expire_interval;
      size_t _expire_limit;
      time_t _last_expire;
      double _deferred_interval;
      double _last_dispatch;
      size_t _byte_limit;
      time_t _recover_dead_interval;
      time_t _last_recover_dead;
  }; // class Exchange

  std::ostream &operator<<(std::ostream &ss, const Exchange *exch);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
