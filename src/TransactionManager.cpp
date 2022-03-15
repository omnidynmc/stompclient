#include "config.h"

#include <string>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <openframe/openframe.h>

#include "Transaction.h"
#include "TransactionManager.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** TransactionManager Class                                             **
 **************************************************************************/

  TransactionManager::TransactionManager() {
  } // TransactionManager::TransactionManager

  TransactionManager::~TransactionManager() {
    destroy_transactions();
  } // TransactionManager::~TransactionManager

  Transaction *TransactionManager::create_transaction(const std::string &key) {
    transaction_itr itr = _transactions.find(key);
    if (itr != _transactions.end()) return itr->second;

    Transaction *trans = new Transaction(key);
    _transactions[key] = trans;

    return trans;
  } // TransactionManager::create_transaction

  Transaction *TransactionManager::find_transaction(const std::string &key) {
    transaction_itr itr = _transactions.find(key);
    if (itr == _transactions.end()) return NULL;
    return itr->second;
  } // TransactionManager::find_transaction

  void TransactionManager::destroy_transactions() {
    while( !_transactions.empty() ) {
      transaction_itr itr = _transactions.begin();
      Transaction *trans = itr->second;
      trans->release();
      _transactions.erase(itr);
    } // while
  } // TransactionManager::destroy_transactions

  bool TransactionManager::destroy_transaction(const std::string &key) {
    transaction_itr itr = _transactions.find(key);
    if (itr == _transactions.end()) return false;
    Transaction *trans = itr->second;
    trans->release();
    _transactions.erase(itr);
    return true;
  } // TransactionManager::destroy_transaction
} // namespace stomp
