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
#include <openstats/openstats.h>

#include "StompMessage.h"
#include "StompPeer.h"
#include "StompServer.h"
#include "Subscription.h"

namespace stomp {
  using namespace openframe;
  using namespace openframe::loglevel;
  using namespace std;

/**************************************************************************
 ** StompPeer Class                                                       **
 **************************************************************************/

  StompPeer::StompPeer(const int sock) :
      _disconnect(false),
      _sock(sock),
      _intval_logstats(StompServer::kDefaultLogstatsInterval) {

    _ip = SocketBase::derive_peer_ip(_sock);
    stringstream s;
    s << SocketBase::derive_peer_port(_sock);
    _port = s.str();
    s.str("");
    s << _ip << ":" << _port;
    _uniq_id = _peer_str = s.str();
    _authenticated = false;

    try {
      _logstats = new Stopwatch(_intval_logstats);
    } // try
    catch(bad_alloc xa) {
      assert(false);
    } // catch

    return;
  } // StompPeer::StompPeer

  StompPeer::StompPeer(const string &host, const int port) :
      _disconnect(false),
      _sock(0),
      _intval_logstats(StompServer::kDefaultLogstatsInterval) {

    _ip = host;
    stringstream s;
    s << port;
    _port = s.str();
    s.str("");
    s << _ip << ":" << _port;
    _uniq_id = _peer_str = s.str();
    _authenticated = false;

    try {
      _logstats = new Stopwatch(_intval_logstats);
    } // try
    catch(bad_alloc xa) {
      assert(false);
    } // catch

    return;
  } // StompPeer::StompPeer

  StompPeer::~StompPeer() {
    delete _logstats;
    return;
  } // StompPeer::~StompPeer

  const bool StompPeer::run() {
    bool didWork = false;
    checkLogstats();
    didWork = process();

    return didWork;
  } // StompPeer::run

  const bool StompPeer::checkLogstats() {
    if (!_logstats->Next()) return false;

    double fps_in = double(_stats.num_frames_in) / double(time(NULL) - _stats.last_report_at);
    double fps_out = double(_stats.num_frames_out) / double(time(NULL) - _stats.last_report_at);
    double kbps = (double(_stats.num_bytes_in) / double(time(NULL) - _stats.last_report_at)) / 1024.0;
    double kb = double(_stats.num_bytes_in) / double(1024);

    LOG(LogNotice, << "Stats peer "
                   << _peer_str
                   << ", sentQ " << _sentQ.size()
                   << ", sent " << std::setprecision(2) << std::fixed << float(_stats.num_frames_out)/1000 << "k"
                   << ", fps in " << std::setprecision(2) << std::fixed << fps_in << "/s"
                   << ", fps out " << std::setprecision(2) << std::fixed << fps_out << "/s, bytes "
                   << kb << "k, "
                   << kbps << " Kbps"
                   << std::endl);

    for(binds_citr citr=_binds.begin(); citr != _binds.end(); citr++) {
      binds_citr ncitr = citr;
      ncitr++;
      char c = (ncitr == _binds.end() ? '`' : '|');
      Subscription *sub = (*citr);
      LOG(LogNotice, << "  " << c << "- Subscription " << sub->key()
                     << " " << sub->toStats() << std::endl);
    } // for

    init_stats();
    return true;
  } // StompPeer::checkLogstats

  void StompPeer::wantDisconnect() {
    _disconnect = true;
  } // StompPeer::wantDisconnect

  bool StompPeer::auth(const std::string &login, const std::string &passcode) {
    onDestroyStats();
    set_stat_id_prefix(login);
    _uniq_id = login + "@" +_uniq_id;
    onDescribeStats();
    _authenticated = true;
    return true;
  } // StompPeer::auth

  void StompPeer::onBind(Subscription *sub) {
    LOG(LogInfo, << "StompPeer bound to " << sub << std::endl);
  } // onBind

  void StompPeer::onUnbind(Subscription *sub) {
    LOG(LogInfo, << "StompPeer unbound from " << sub << std::endl);
  } // unBind

  void StompPeer::onRecoverableError(StompFrame *frame) {
    LOG(LogInfo, << "StompPeer error "
                 << frame->get_header("message") << " to " << this << std::endl);
  } // StompPeer::onRecoverableError

  void StompPeer::onFatalError(StompFrame *frame) {
    LOG(LogInfo, << "StompPeer fatal error "
                 << frame->get_header("message") << " to " << this << std::endl);
    wantDisconnect();
  } // StompPeer::onFatalError

  std::ostream &operator<<(std::ostream &ss, const StompPeer *peer) {
    ss << peer->toString();
    return ss;
  } // operator<<
} // namespace stomp

