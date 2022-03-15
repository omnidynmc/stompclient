#ifndef __LIBSTOMP_FEED_H
#define __LIBSTOMP_FEED_H

#include <string>
#include <vector>
#include <list>

#include <openframe/openframe.h>
#include <stomp/StompClient.h>

namespace stomp {
/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/


/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/
  class Feed_Exception : public openframe::OpenFrame_Exception {
    public:
      Feed_Exception(const std::string message) throw() : openframe::OpenFrame_Exception(message) { };
  }; // class Feed_Exception

  class Feed : public stomp::StompClient {
    public:
      typedef stomp::StompClient super;
      Feed(const std::string &hosts,
           const std::string &login,
           const std::string &passcode,
           const std::string &dest_in="",
           const std::string &dest_out="");
      virtual ~Feed();

      void post(const std::string &msg);
      void post(const std::string &dest, const std::string &msg);

      void onConnect(const openframe::Connection *con);
      void onTryConnect(const std::string &host, const int port);
      void onConnectError(const std::string &host, const int port, const char *error);
      void onDisconnect(const openframe::Connection *con);
      void onConnectTimeout(const openframe::Peer *peer);
      void onRecoverableError(stomp::StompFrame *frame);
      void onFatalError(stomp::StompFrame *frame);
      void onPeerStats(const openframe::Peer *peer);
      std::string sub_id();
      void set_sub_id(const std::string &sub_id);
      inline std::string dest_in() const { return _dest_in; }

    protected:
    private:
      std::string _dest_in;
      std::string _dest_out;
      std::string _sub_id;
      openframe::OFLock _sub_id_l;
  }; // Feed

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/
} // namespace openaprs
#endif
