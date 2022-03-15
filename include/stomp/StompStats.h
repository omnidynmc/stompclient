#ifndef LIBSTOMP_STOMPSTATS_H
#define LIBSTOMP_STOMPSTATS_H

#include <sstream>
#include <string>
#include <queue>

#include "StompMessage.h"
#include "StompClient.h"
#include "ExchangeManager.h"

#include <openframe/openframe.h>
#include <openstats/StatsClient.h>

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompStats_Exception : public openframe::OpenFrame_Exception {
    public:
      StompStats_Exception(const std::string message) throw() : OpenFrame_Exception(message) { };
  }; // class StompStats_Exception

  // ### Forward Delcarations ###
  class StompFrame;
  class StompMessage;
  class StompStats : public StompClient,
                     public openstats::StatsClient {
    public:
      typedef StompClient super;

      StompStats(
        const std::string &source,
        const std::string &instance,
        const time_t sampling_freq,
        const int max_queued_messages,
        const std::string &hosts,
        const std::string &login,
        const std::string &passcode,
        const std::string &dest="/topic/stats"
      );
      virtual ~StompStats();

      virtual void stop();
      virtual void onConnect(const openframe::Connection *con);
      virtual void onBind(Subscription *sub);
      virtual void onUnbind(Subscription *sub);
      virtual void onRecoverableError(StompFrame *frame);
      virtual void onFatalError(StompFrame *frame);
      virtual void onStats();

    protected:
      // ### Protected Members ###

    private:
      std::string _source;
      std::string _instance;
      int _max_queued_messages;

      std::string _destination;
  }; // StompStats

  std::ostream &operator<<(std::ostream &ss, const StompStats *sclient);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/
} // namespace stomp
#endif
