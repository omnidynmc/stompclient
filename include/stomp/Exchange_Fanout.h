#ifndef __MODULE_STOMP_EXCHANGE_FANOUT_H
#define __MODULE_STOMP_EXCHANGE_FANOUT_H


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

  class Exchange_Fanout : public Exchange {
    public:
      Exchange_Fanout(const std::string &key);
      virtual ~Exchange_Fanout();

      virtual const std::string toString() const;
      virtual size_t onDispatch(const size_t limit);

    protected:
    private:
      unsigned int _index;
  }; // class Exchange

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
