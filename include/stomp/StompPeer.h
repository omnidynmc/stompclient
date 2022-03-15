#ifndef LIBSTOMP_STOMPPEER_H
#define LIBSTOMP_STOMPPEER_H

#include <sstream>
#include <set>
#include <map>
#include <queue>
#include <string>

#include <openframe/openframe.h>
#include <openstats/openstats.h>

#include "StompServer.h"
#include "StompParser.h"
#include "StompFrame.h"
#include "StompMessage.h"
#include "Subscription.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/
#define DEVICE_BINARY_SIZE  32
#define MAXPAYLOAD_SIZE     256

#define STOMP_FRAME_ARG		StompFrame *frame

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/
  class StompMessage;

  class StompPeer_Exception : public openframe::OpenFrame_Exception {
    public:
      StompPeer_Exception(const std::string message) throw() : OpenFrame_Exception(message) { };
  }; // class StompPeer_Exception

  class StompPeer : public StompParser,
                    public openframe::ConnectionManager {
    public:
      StompPeer(const int);
      StompPeer(const std::string &host, const int port);
      virtual ~StompPeer();

      friend class StompServer;

      /*************
       ** Members **
       *************/
      inline const int sock() const { return _sock; }
      inline bool authenticated() const { return _authenticated; }

      inline StompPeer &intval_logstats(const time_t intval_logstats) {
        _intval_logstats = intval_logstats;
        delete _logstats;
        _logstats = new openframe::Stopwatch(_intval_logstats);
        return *this;
      } // intval_logstats
      inline const time_t intval_logstats() const { return _intval_logstats; }

      // ### Public Members - Bindings ###
      virtual void onBind(Subscription *);
      virtual void onUnbind(Subscription *);
      virtual void onRecoverableError(StompFrame *frame);
      virtual void onFatalError(StompFrame *frame);
      virtual bool auth(const std::string &login, const std::string &passcode);

      // ### ConnectionManager pure virtuals ###
      virtual void onConnect(const openframe::Connection *) { }
      virtual void onDisconnect(const openframe::Connection *) { }
      virtual void onRead(const openframe::Peer *) { }
      virtual const std::string::size_type onWrite(const openframe::Peer *, std::string &ret) { return 0; }

      // ### Stats Pure Virtuals ###
      //virtual void onDescribeStats();
      //virtual void onDestroyStats();

      const bool run();
      const bool disconnect() const { return _disconnect; }
      void wantDisconnect();

      inline const std::string toString() const {
        return _peer_str;
      } // toString

      const bool checkLogstats();

    protected:
      // ### Private Members ###
      openframe::Stopwatch *_logstats;

      std::string _ip;
      std::string _port;
      std::string _peer_str;

      bool _disconnect;
      int _sock;
      time_t _intval_logstats;
      time_t _time_connected;
      bool _authenticated;
  }; // StompPeer

  std::ostream &operator<<(std::ostream &ss, const StompPeer *peer);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
