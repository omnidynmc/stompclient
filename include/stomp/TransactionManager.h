#ifndef LIBSTOMP_TRANSACTIONMANAGER_H
#define LIBSTOMP_TRANSACTIONMANAGER_H

#include <map>
#include <string>

#include <openframe/openframe.h>
#include <openstats/StatsClient_Interface.h>

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class Transaction;
  class TransactionManager {
    public:
      typedef std::map<std::string, Transaction *> transaction_t;
      typedef transaction_t::iterator transaction_itr;
      typedef transaction_t::const_iterator transaction_citr;
      typedef transaction_t::size_type transaction_st;

      TransactionManager();
      virtual ~TransactionManager();

      Transaction *create_transaction(const std::string &key);
      Transaction *find_transaction(const std::string &key);
      bool destroy_transaction(const std::string &key);
      void destroy_transactions();

    protected:
    private:
      transaction_t _transactions;
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
