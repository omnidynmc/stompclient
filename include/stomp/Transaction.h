#ifndef LIBSTOMP_TRANSACTION_H
#define LIBSTOMP_TRANSACTION_H

#include <deque>
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

  class StompFrame;
  class Transaction : public openframe::Refcount {
    public:
      typedef std::deque<StompFrame *> queue_t;
      typedef queue_t::size_type queue_st;

      Transaction(const std::string &key);
      virtual ~Transaction();

      void store(StompFrame *frame);
      queue_st dequeue(queue_t &ret);
      void purge_all();

    protected:
    private:
      std::string _key;
      queue_t _queue;
  }; // class Transaction

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
