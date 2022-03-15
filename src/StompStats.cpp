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
#include "StompStats.h"

namespace stomp {
  using namespace openframe::loglevel;

/**************************************************************************
 ** StompStats Class                                                     **
 **************************************************************************/

  StompStats::StompStats(
                         const std::string &source,
                         const std::string &instance,
                         const time_t sampling_freq,
                         const int max_queued_messages,
                         const std::string &hosts,
                         const std::string &login,
                         const std::string &passcode,
                         const std::string &dest
                        )
             : super(hosts, login, passcode),
               openstats::StatsClient(sampling_freq),
               _source(source),
               _instance(instance),
               _max_queued_messages(max_queued_messages) {
    if (instance == "combined" || source == "combined")
      throw StompStats_Exception("neither instance or source may be called 'combined'");

    _destination = dest + "." + _source + "." + _instance;
    set_connect_read_timeout(0);
  } // StompStats::StompStats

  StompStats::~StompStats() {
  } // StompStats::~StompStats

  void StompStats::stop() {
    StatsClient::stop();
    StompClient::stop();
  } // StompStats::stop

  void StompStats::onConnect(const openframe::Connection *con) {
    StompClient::onConnect(con);
  } // StompStats::onConnect

  void StompStats::onBind(Subscription *sub) {
  } // StompStats::onBind

  void StompStats::onUnbind(Subscription *sub) {
  } // StompStats::onUnbind

  void StompStats::onRecoverableError(StompFrame *frame) {
  } // StompStats::onRecoverableError

  void StompStats::onFatalError(StompFrame *frame) {
  } // StompStats::onFatalError

  void StompStats::onStats() {
    if (!is_ready()) return;

    openframe::scoped_lock slock(&_descrips_l);
    for(descrip_itr itr = _descrips.begin(); itr != _descrips.end(); itr++ ) {
      openstats::Data_Abstract *data = itr->second;
      stringstream out;
//      out << "Sending stats " << data->toString()
//                << ",sampling_freq=" << sampling_freq();
      out << data->toJson();
      StompFrame *frame = new StompFrame("SEND", out.str());
      frame->add_header("destination", _destination);
      frame->add_header("StompStats-Source", _source);
      frame->add_header("StompStats-Instance", _instance);
      send_frame(frame);
      frame->release();
      data->reset();
    } // for
  } // StompStats::onStats

} // namespace stomp
