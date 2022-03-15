#ifndef __LIBSTOMP_STOMPCLIENT_H
#define __LIBSTOMP_STOMPCLIENT_H

#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <set>

#include <netdb.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "StompMessage.h"
#include "StompParser.h"
#include "ExchangeManager.h"

#include <openframe/openframe.h>

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompClient_Exception : public openframe::OpenFrame_Exception {
    public:
      StompClient_Exception(const std::string message) throw() : OpenFrame_Exception(message) { };
  }; // class StompClient_Exception

  // ### Forward Delcarations ###
  class StompFrame;
  class StompMessage;
  class StompClient : public StompParser,
                      public openframe::PeerController {
    public:
      typedef StompParser super;

      static const time_t DEFAULT_INTVAL_LOGSTATS;
      static const time_t CONNECT_RETRY_INTERVAL;

      StompClient(const std::string &hosts,
                  const std::string &login="",
                  const std::string &passcode="",
                  const time_t connect_retry_interval=CONNECT_RETRY_INTERVAL);
      virtual ~StompClient();

      virtual StompClient &start();
      virtual void stop();

      // ### Options ###
      inline StompClient &max_work(const size_t max_work) {
        _max_work = max_work;
        return *this;
      } // max_work
      inline size_t max_work() const { return _max_work; }
      inline StompClient &intval_logstats(const time_t intval_logstats) {
        _intval_logstats = intval_logstats;
        return *this;
      } // intval_logstats
      inline size_t intval_logstats() const { return _intval_logstats; }
      inline bool debug() const { return _debug; }
      inline StompClient &debug(const bool debug) {
        _debug = debug;
        return *this;
      } // debug

      // ### StompPraser pure virtuals ###
      virtual void onBind(Subscription *sub);
      virtual void onUnbind(Subscription *sub);
      virtual void onRecoverableError(StompFrame *frame);
      virtual void onFatalError(StompFrame *frame);

      // ### ConnectionManager virtuals ###
      virtual void onConnect(const openframe::Connection *);
      virtual void onConnectTimeout(const openframe::Peer *peer);
      virtual void onDisconnect(const openframe::Connection *);
      virtual void onRead(const openframe::Peer *);
      virtual const string::size_type onWrite(const openframe::Peer *, string &);

      virtual std::string toString() const {
        std::stringstream out;
        out << peer_str;
        return out.str();
      } // toString

      bool is_ready();
    protected:
      void set_ready(const bool ready);

      // ### Protected Members ###
      std::string _login;
      std::string _passcode;

      bool _debug;
      time_t _intval_logstats;
      time_t _connect_retry_interval;
      size_t _max_work;
      bool _ready;
      openframe::OFLock _ready_l;

      std::string peer_str;
    private:
  }; // StompClient

  std::ostream &operator<<(std::ostream &ss, const StompClient *sclient);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/
} // namespace stomp
#endif
