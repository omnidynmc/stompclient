#ifndef LIBSTOMP_STOMP_H
#define LIBSTOMP_STOMP_H

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

  // ### Forward Delcarations ###
  class StompHeaders;
  class StompFrame;
  class Stomp : public StompParser,
                private openframe::SocketBase {
    public:
      typedef StompParser super;

      Stomp(const std::string &host, const std::string &login="",
            const std::string &passcode="",
            StompHeaders *headers=NULL);
      virtual ~Stomp();

      void init();

      std::string connected_to() const;
      bool is_connected() const;
      std::string last_error() const { return error(); }

      virtual bool next_frame(StompFrame *&frame);

      virtual void onRecoverableError(StompFrame *frame) { };
      virtual void onFatalError(StompFrame *frame) { };
      virtual void onBind(Subscription *sub) { };
      virtual void onUnbind(Subscription *sub) { };

    protected:
      typedef std::vector<openframe::Host *> hosts_t;
      typedef hosts_t::size_type hosts_st;

      bool try_connect();
      bool must_bail();
      openframe::Host *rotate_server();
      virtual const string::size_type _write(const string &buf);
      int read_socket(int sock);

      // constructor vars
      std::string _hosts_str;
      std::string _login;
      std::string _passcode;
      size_t _host_index;
      StompHeaders *_headers;

      hosts_t _hosts;
      int _sock;
      openframe::Peer *_peer;

      fd_set _r_set;
    private:
  }; // Stomp

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/
} // namespace stomp
#endif
