#include <config.h>

#include <string>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <openframe/openframe.h>

#include "Push.h"

namespace stomp {
  using namespace openframe::loglevel;

  Push::Push(const std::string &hosts,
             const std::string &login,
             const std::string &passcode)
       : super(hosts, login, passcode) {

    // FIXME
    set_peer_stats_interval(10);
  } // Push::Push

  Push::~Push() {
  } // Push:~Push

  void Push::post(const std::string &dest, const std::string &msg) {
    if (!dest.length()) throw Push_Exception("no destination given");
    if (!is_connected()) return;

    stomp::StompFrame *frame = new stomp::StompFrame("SEND", msg);
    frame->add_header("destination", dest);
    send_frame(frame);
    frame->release();
  } // Push::post

  void Push::onConnect(const openframe::Connection *con) {
    stomp::StompClient::onConnect(con);
    LOG(LogNotice, << "Stomp push #"
                   << num_connects()
                   << " connected to " << con->peer_str << std::endl);
  } // StompClient::onConnect

  void Push::onDisconnect(const openframe::Connection *con) {
    stomp::StompClient::onDisconnect(con);
    LOG(LogNotice, << "Stomp push #"
                   << num_connects()
                   << " disconnected from " << con->peer_str << std::endl);
  } // Push::onDisconnect

  void Push::onTryConnect(const std::string &host, const int port) {
    LOG(LogNotice, << "Stomp push #"
                   << num_connects()
                   << " trying to connect to "
                   << host << ":" << port
                   << std::endl);
  } // Push::onTryConnect

  void Push::onConnectError(const std::string &host, const int port, const char *error) {
    LOG(LogNotice, <<"Stomp push #"
                   << num_connects()
                   << " error \"" << error << "\""
                   << " connecting to "
                   << host << ":" << port
                   << std::endl);
  } // Push::onConnectError

  void Push::onConnectTimeout(const openframe::Peer *peer) {
    stomp::StompClient::onConnectTimeout(peer);
    LOG(LogNotice, << "Stomp push #"
                   << num_connects()
                   << " connection timed out with " << peer->peer_str << std::endl);
  } // Push::onConnectTimeout

  void Push::onPeerStats(const openframe::Peer *peer) {
    LOG(LogNotice, << peer->peer_stats_str() << std::endl);
  } // Push::onPeerStats

  void Push::onRecoverableError(stomp::StompFrame *frame) {
    LOG(LogWarn, << "ERROR " << frame << std::endl);
    LOG(LogWarn, << "ERROR Message: " << frame->get_header("message", "undefined") << std::endl);
  } // Push::onRecoverableError

  void Push::onFatalError(stomp::StompFrame *frame) {
    LOG(LogWarn, << "ERROR " << frame << std::endl);
    LOG(LogWarn, << "ERROR Message: " << frame->get_header("message", "undefined") << std::endl);
  } // Push::onFatalError
} // namespace openaprs
