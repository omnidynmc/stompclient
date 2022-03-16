#include "config.h"

#include <string>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <openframe/openframe.h>

#include "Exchange.h"
#include "Exchange_Fanout.h"
#include "Exchange_Topic.h"
#include "ExchangeManager.h"
#include "Subscription.h"
#include "StompPeer.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** Exchange Class                                                       **
 **************************************************************************/
  const size_t ExchangeManager::kDispatchLimit	= 100;

  ExchangeManager::ExchangeManager() {

  } // ExchangeManager::ExchangeManager

  ExchangeManager::~ExchangeManager() {
    forget_subscriptions();
    destroy_exchanges();
  } // ExchangeManager::~ExchangeManager

  void ExchangeManager::onDescribeStats() {
  } // ExchangeManager::onDescribeStats

  void ExchangeManager::onDestroyStats() {
    destroy_stat("*");
  } // ExchangeManager::onDestroyStats

  Exchange *ExchangeManager::create_exchange(const std::string &key, const Exchange::exchangeTypeEnum exchange_type) {
    assert( key.length() ); 	// bug
    exchange_itr itr = _exchanges.find(key);
    if (itr != _exchanges.end()) {
//      log(LogInfo) << "ExchangeManager found existing key; " << key << std::endl;
      return itr->second;
    } // if

    Exchange *exch;
    try {
      switch(exchange_type) {
        case Exchange::exchangeTypeFanout:
          exch = new Exchange_Fanout(key);
          break;
        case Exchange::exchangeTypeTopic:
          exch = new Exchange_Topic(key);
          break;
        case Exchange::exchangeTypeDirect:
        case Exchange::exchangeTypeHeaders:
        default:
          assert(false);	// bug
      } // switch
      exch->elogger( elogger(), elog_name() );
      std::string safe_key = key;
      openframe::StringTool::replace("/", ".", safe_key);
      exch->replace_stats(stats(), "libstomp.exchanges."+safe_key);
    } // try
    catch(std::bad_alloc &xa) {
      assert(false);
    } // catch

    LOG(LogInfo, << "ExchangeManager created " << exch << std::endl);
    _exchanges.insert( make_pair(key, exch) );
    match_subscriptions();
    return exch;
  } // ExchangeManager::create_exchange

  bool ExchangeManager::destroy_exchange(const std::string &key) {
    assert( key.length() );
    exchange_itr itr = _exchanges.find(key);
    if (itr == _exchanges.end()) return false;
    Exchange *exch = itr->second;
    LOG(LogInfo, << "ExchangeManager destroyed " << exch << std::endl);
    _exchanges.erase( key );
    exch->release();
    return true;
  } // ExchangeManager::destroy_exchange

  void ExchangeManager::destroy_exchanges() {
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      std::string key = itr->first;
      Exchange *exch = itr->second;
      LOG(LogInfo, << "ExchangeManager destroyed " << exch << std::endl);
      exch->release();
    } // for
    _exchanges.clear();
  } // ExchangeManager::destroy_exchanges

  void ExchangeManager::dispatch_exchanges() {
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      Exchange *exch = itr->second;
      size_t num_dispatched = exch->dispatch(kDispatchLimit);
      exch->try_stats();
    } // for
  } // ExchangeManager::dispatch_exchanges

  void ExchangeManager::match_subscriptions() {
    for(subscriptions_itr itr = _subscriptions.begin(); itr != _subscriptions.end(); itr++) {
      Subscription *sub = *itr;
      subscribe(sub);
    } // for
  } // ExchangeManager::match_subscriptions

  bool ExchangeManager::store_subscription(Subscription *sub) {
    subscriptions_citr citr = _subscriptions.find(sub);
    if (citr != _subscriptions.end()) return false;
    sub->retain();
    _subscriptions.insert(sub);
    LOG(LogInfo, << "ExchangeManager stored " << sub << std::endl);
    return true;
  } // EchangeManager::store_subscription

  bool ExchangeManager::forget_subscription(Subscription *sub) {
    subscriptions_citr citr = _subscriptions.find(sub);
    if (citr == _subscriptions.end()) return false;
    _subscriptions.erase(sub);
    LOG(LogInfo, << "ExchangeManager forgot " << sub << std::endl);
    sub->release();
    return true;
  } // EchangeManager::forget_subscription

  void ExchangeManager::forget_subscriptions() {
    for(subscriptions_itr itr = _subscriptions.begin(); itr != _subscriptions.end(); itr++) {
      Subscription *sub = *itr;
      LOG(LogInfo, << "ExchangeManager forgot " << sub << std::endl);
      sub->release();
    } // for
    _subscriptions.clear();
  } // ExchangeManager::forget_subscriptions

  void ExchangeManager::forget_subscriptions(StompPeer *peer) {
    std::deque<Subscription *> rlist;
    for(subscriptions_itr itr = _subscriptions.begin(); itr != _subscriptions.end(); itr++) {
      Subscription *sub = *itr;
      if (!sub->is_peer(peer)) continue;
      rlist.push_back(sub);
    } // for

    while( !rlist.empty() ) {
      Subscription *sub = rlist.front();
      _subscriptions.erase(sub);
      LOG(LogInfo, << "ExchangeManager forgot " << sub << std::endl);
      sub->release();
      rlist.pop_front();
    } // while
  } // ExchangeManager::forget_subscriptions

  ExchangeManager::exchange_st ExchangeManager::subscribe(Subscription *sub) {
    exchange_st num = 0;
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      Exchange *exch = itr->second;
//      log(LogInfo) << "ExchangeManager checking " << sub << " against " << exch << std::endl;
      bool ok = sub->match(itr->first);
      if (!ok) continue;
      exch->bind(sub);
      // if (bound) log(LogInfo) << "ExchangeManager binding " << sub << " to " << exch << std::endl;
      num++;
    } // for
    store_subscription(sub);
    return num++;
  } // ExchangeManager::subscribe

  Subscription *ExchangeManager::find_subscription(StompPeer *peer, const std::string &id) {
    for(subscriptions_itr itr = _subscriptions.begin(); itr != _subscriptions.end(); itr++) {
      Subscription *sub = *itr;
      if ( sub->is_id(id) && sub->is_peer(peer) ) return sub;
    } // for

    return NULL;
  } // ExchangeManager::find_subscription

/*
  ExchangeManager::exchange_st ExchangeManager::unsubscribe(const std::string &id) {
    exchange_st num = 0;
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      Exchange *exch = itr->second;
      num += exch->unbind(id);
    } // for
    return num++;
  } // ExchangeManager::unsubscribe
*/
  ExchangeManager::exchange_st ExchangeManager::unsubscribe(Subscription *sub) {
    exchange_st num = 0;
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      Exchange *exch = itr->second;
      num += exch->unbind(sub);
    } // for
    forget_subscription(sub);
    return num++;
  } // ExchangeManager::unsubscribe

  ExchangeManager::exchange_st ExchangeManager::unsubscribe(StompPeer *peer) {
    exchange_st num = 0;
    for(exchange_itr itr = _exchanges.begin(); itr != _exchanges.end(); itr++) {
      Exchange *exch = itr->second;
      num += exch->unbind(peer);
    } // for
    forget_subscriptions(peer);
    return num++;
  } // ExchangeManager::unsubscribe

//  std::ostream &operator<<(std::ostream &ss, const Exchange *exch) {
//    ss << exch->toString();
//    return ss;
//  } // operator<<
} // namespace stomp
