#ifndef __MODULE_STOMP_EXCHANGEMANAGER_H
#define __MODULE_STOMP_EXCHANGEMANAGER_H

#include <map>
#include <string>

#include <openframe/openframe.h>
#include <openstats/StatsClient_Interface.h>

#include "Exchange.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompPeer;
  class ExchangeManager : public openframe::OpenFrame_Abstract,
                          public openframe::Refcount,
                          public openstats::StatsClient_Interface {
    public:
      typedef std::map<std::string, Exchange *> exchange_t;
      typedef exchange_t::iterator exchange_itr;
      typedef exchange_t::const_iterator exchange_citr;
      typedef exchange_t::size_type exchange_st;

      typedef std::set<Subscription *> subscriptions_t;
      typedef subscriptions_t::iterator subscriptions_itr;
      typedef subscriptions_t::const_iterator subscriptions_citr;
      typedef subscriptions_t::size_type subscriptions_st;

      static const size_t kDispatchLimit;

      ExchangeManager();
      virtual ~ExchangeManager();
      void onDescribeStats();
      void onDestroyStats();

      Exchange *create_exchange(const std::string &key, const Exchange::exchangeTypeEnum exchange_type);
      bool destroy_exchange(const std::string &key);
      void destroy_exchanges();
      exchange_st subscribe(Subscription *sub);
      exchange_st unsubscribe(Subscription *sub);
      //exchange_st unsubscribe(const string &id);
      exchange_st unsubscribe(StompPeer *peer);
      exchange_st size() const { return _exchanges.size(); }

      bool store_subscription(Subscription *sub);
      bool forget_subscription(Subscription *sub);
      Subscription *find_subscription(StompPeer *peer, const std::string &id);
      void forget_subscriptions();
      void forget_subscriptions(StompPeer *peer);
      void match_subscriptions();

      void dispatch_exchanges();

    protected:
    private:
      exchange_t _exchanges;
      subscriptions_t _subscriptions;
  }; // class Exchange

  //std::ostream &operator<<(std::ostream &ss, const Exchange *exch);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
