#include "config.h"

#include <string>
#include <cassert>
#include <cstring>
#include <list>
#include <queue>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <openframe/openframe.h>

#include "StompFrame.h"
#include "Transaction.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** Transaction Class                                                    **
 **************************************************************************/
  Transaction::Transaction(const std::string &key)
              : _key(key) {
  } // Transaction::Transaction

  Transaction::~Transaction() {
    purge_all();
  } // Transaction::~Transaction

  void Transaction::store(StompFrame *frame) {
    frame->retain();
    frame->set_execute_transaction(true);
    _queue.push_back(frame);
  } // Transaction::store

  Transaction::queue_st Transaction::dequeue(queue_t &ret) {
    queue_st num = _queue.size();
    while( !_queue.empty() ) {
      StompFrame *frame = _queue.front();
      ret.push_front(frame);
      _queue.pop_front();
    } // while

    return num;
  } // Transaction::dequeue

  void Transaction::purge_all() {
    while( !_queue.empty() ) {
      StompFrame *frame = _queue.front();
      frame->release();
      _queue.pop_front();
    } // while
  } // Transaction::purge_all
} // namespace stomp
