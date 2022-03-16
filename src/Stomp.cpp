#include <fstream>
#include <string>
#include <queue>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <fstream>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <openframe/openframe.h>

#include "StompMessage.h"
#include "StompClient.h"
#include "StompHeaders.h"
#include "Stomp.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** Stomp Class                                                          **
 **************************************************************************/

  Stomp::Stomp(const std::string &hosts, const std::string &login,
               const std::string &passcode,
               StompHeaders *headers)
        : _hosts_str(hosts),
          _login(login),
          _passcode(passcode),
          _headers(headers),
          _peer(NULL) {
    init();
  } // Stomp::Stomp

  Stomp::~Stomp() {
    for(hosts_st i=0; i < _hosts.size(); i++) {
      delete _hosts[i];
    } // for

    if (_headers) _headers->release();
    if (_peer) delete _peer;
  } // Stomp::~Stomp

  void Stomp::init() {
    openframe::StringToken st;
    st.setDelimiter(',');
    st = _hosts_str;

    if (st.size() < 1) throw openframe::SocketBase_Exception("invalid hosts list");

    for(size_t i=0; i < st.size(); i++) {
      openframe::Host *host = new openframe::Host(st[i]);
      _hosts.push_back(host);
    } // for

    _host_index = 0;
    try_connect();
  } // Stomp::init

  bool Stomp::must_bail() {
    if (_peer) {
      close(_peer->sock);
      delete _peer, _peer=NULL;
    } // if
    return false;
  } // Stomp::must_bail

  bool Stomp::try_connect() {
    if (_peer) return true;

    try {
      _peer = new openframe::Peer;
    } // try
    catch(std::bad_alloc &xa) {
      assert(false);
    } // catch

    openframe::Host *host = rotate_server();

    bool ok = SocketBase::connect(_peer, host->host(), host->port(), ""); // FIXME bindip
    if (!ok) return must_bail();

    StompHeaders *headers = new StompHeaders();
    if (_headers) headers->copy_headers_from(_headers);

    ok = StompParser::connect(_login, _passcode, headers);
    if (!ok) return must_bail();

    StompFrame *frame;
    for(int i=0; i < 5; i++) {
      try {
        ok = next_frame(frame);
      } // try
      catch(Stomp_Exception &ex) {
        ok = false;
      } // catch

      if (ok) {
        if (frame->is_command("CONNECTED")) {
          frame->release();
          return true;
        } // if
        else return must_bail();
      } // if
      sleep(2);
    } // for

    return must_bail();
  } // Stomp::try_connect

  openframe::Host *Stomp::rotate_server() {
    assert(_hosts.size() != 0);
    unsigned int index = _host_index++ % _hosts.size();
    return _hosts[index];
  } // Stomp::rotate_server

  std::string Stomp::connected_to() const {
    return _peer ? _peer->peer_str : "not connected";
  } // Stomp::connected_to

  bool Stomp::is_connected() const {
    return _peer ? true : false;
  } // Stomp::is_connected

  bool Stomp::next_frame(StompFrame *&frame) {
    // this allows us to consume the rest of the buffer
    if (super::next_frame(frame)) return true;

    if (!_peer) {
      bool ok = try_connect();
      if (!ok) throw StompNotConnected_Exception();
    } // if

    for(int i=0; i < 100; i++) {
      int ret = read_socket(_peer->sock);
      if (ret < 0) throw StompNotConnected_Exception();
      else if (i == 0) break;
    } // for

    return super::next_frame(frame);
  } // Stomp::next_frame

  int Stomp::read_socket(int sock) {
    bool ok = is_read_ok(sock);
    if (!ok) return 0;

    char packet[SOCKETBASE_MTU];
    SocketBase::socketBaseReturn_t ret = SocketBase::read(sock, packet, SOCKETBASE_MTU);

    if (ret < 1) return must_bail();
    receive(packet, ret);

    return ret;
  } // Stomp::read_socket

  const string::size_type Stomp::_write(const string &buf) {
    bool ok;
    if (_peer == NULL) {
      ok = try_connect();
      if (!ok) return 0;
    } // if

    ok = is_write_ok(_peer->sock);
    if (!ok) return 0;
    int len = SocketBase::write(_peer->sock, buf.data(), buf.length());
    if (len < 1) return must_bail();
    return len;
  } // Stomp::_write
} // namespace stomp
