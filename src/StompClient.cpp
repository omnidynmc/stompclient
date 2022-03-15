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
#include "StompPeer.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** StompClient Class                                                    **
 **************************************************************************/
  const time_t StompClient::CONNECT_RETRY_INTERVAL       = 15;
  const time_t StompClient::DEFAULT_INTVAL_LOGSTATS     = 3600;

  StompClient::StompClient(const std::string &hosts,
                           const std::string &login,
                           const std::string &passcode,
                           const time_t connect_retry_interval)
              : openframe::PeerController(hosts),
                _login(login),
                _passcode(passcode),
                _connect_retry_interval(connect_retry_interval),
                _ready(false) {
  } // StompClient::StompClient

  StompClient::~StompClient() {
  } // StompClient::~StompClient

  StompClient &StompClient::start() {
    openframe::PeerController::init();
    openframe::PeerController::start();
    return *this;
  } // StompClient::start

  void StompClient::stop() {
    openframe::PeerController::stop();
  } // StompClient::stop

  void StompClient::onBind(Subscription *sub) {
  } // StompClient::onBind

  void StompClient::onUnbind(Subscription *sub) {
  } // StompClient::onUnbind

  void StompClient::onRecoverableError(StompFrame *frame) {
  } // StompClient::onRecoverableError

  void StompClient::onFatalError(StompFrame *frame) {
  } // StompClient::onFatalError

  bool StompClient::is_ready() {
    openframe::scoped_lock slock(&_ready_l);
    return _ready;
  } // StompClient::is_ready

  void StompClient::set_ready(const bool ready) {
    openframe::scoped_lock slock(&_ready_l);
    _ready = ready;
  } // StompClient::set_ready

  void StompClient::onConnect(const openframe::Connection *con) {
    openframe::PeerController::onConnect(con);
    peer_str = con->peer_str;
    reset();

    // send connect line
    stomp::StompParser::connect(_login, _passcode);

    set_ready(true);
  } // StompClient::onConnect

  void StompClient::onConnectTimeout(const openframe::Peer *peer) {
  } // StompClient::onConnectTimeout

  void StompClient::onDisconnect(const openframe::Connection *con) {
    PeerController::onDisconnect(con);
    set_ready(false);
  } // StompClient::onDisconnect

  void StompClient::onRead(const openframe::Peer *lis) {
    // already thread safe from StompParser
    receive(lis->in, lis->in_len);
  } // StompClient::onRead

  const string::size_type StompClient::onWrite(const openframe::Peer *lis, std::string &ret) {
    // already thread safe from StompPeer
    transmit(ret);
    return ret.size();
  } // StompClient::onWrite

  std::ostream &operator<<(std::ostream &ss, const StompClient *sclient) {
    ss << sclient->toString();
    return ss;
  } // operator<<
} // namespace stomp
