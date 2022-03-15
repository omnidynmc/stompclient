#ifndef __MODULE_STOMP_EXCHANGE_TOPIC_H
#define __MODULE_STOMP_EXCHANGE_TOPIC_H


#include <set>
#include <queue>
#include <string>

#include <openframe/openframe.h>

#include "Exchange.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class Exchange_Topic : public Exchange {
    public:
      Exchange_Topic(const std::string &key);
      virtual ~Exchange_Topic();

      virtual const std::string toString() const;
      virtual size_t onDispatch(const size_t limit);

    protected:
    private:
  }; // class Exchange

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
