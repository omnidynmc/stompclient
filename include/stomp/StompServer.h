#ifndef LIBSTOMP_STOMPSERVER_H
#define LIBSTOMP_STOMPSERVER_H

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
#include "ExchangeManager.h"

#include <openframe/openframe.h>
#include <openstats/openstats.h>

namespace stomp {
/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/
#define DEVICE_BINARY_SIZE  32
#define MAXPAYLOAD_SIZE     256

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/
  // ### Forward Declarations ###
  class StompFrame;
  class StompMessage;
  class StompPeer;

  class StompServer_Exception : public openframe::OpenFrame_Exception {
    public:
      StompServer_Exception(const std::string message) throw() : openframe::OpenFrame_Exception(message) { };
  }; // class StompServer_Exception

  class StompServer : public openframe::ListenController,
                      public openstats::StatsClient_Interface,
                      public openframe::ThreadMessenger {
    public:
      StompServer(const int, const int, const std::string &bind_ip="");
      virtual ~StompServer();

      /**********************
       ** Type Definitions **
       **********************/
      typedef openframe::ListenController super;

      typedef std::map<int, StompPeer *> peers_t;
      typedef peers_t::iterator peers_itr;
      typedef peers_t::size_type peersSize_t;

      typedef std::set<StompMessage *> topics_t;
      typedef topics_t::iterator topics_itr;
      typedef topics_t::const_iterator topics_citr;
      typedef topics_t::size_type topicsSize_t;

      typedef std::set<StompMessage *> queues_t;
      typedef queues_t::iterator queues_itr;
      typedef queues_t::const_iterator queues_citr;
      typedef queues_t::size_type queuesSize_t;

      /***************
       ** Variables **
       ***************/
      static const time_t kDefaultLogstatsInterval;
      static const time_t kDefaultQueueMessageExpire;
      static const size_t kDefaultMaxWork;
      static const int HEADER_SIZE;

      // ### Core Members ###
      const bool run();
      virtual StompServer &start();

      // ### Options ###
      inline StompServer &max_work(const size_t max_work) {
        _max_work = max_work;
        return *this;
      } // max_work
      inline const size_t max_work() const { return _max_work; }
      inline StompServer &intval_logstats(const time_t intval_logstats) {
        _intval_logstats = intval_logstats;
        return *this;
      } // intval_logstats
      inline const size_t intval_logstats() const { return _intval_logstats; }
      inline StompServer &time_queue_expire(const size_t time_queue_expire) {
        _time_queue_expire = time_queue_expire;
        return *this;
      } // max_work
      inline const time_t time_queue_expire() const { return _time_queue_expire; }
      inline StompServer &set_queue_byte_limit(const size_t queue_byte_limit) {
        _queue_byte_limit = queue_byte_limit;
        return *this;
      } // set_queue_byte_limit

      inline const bool debug() const { return _debug; }
      inline StompServer &debug(const bool debug) {
        _debug = debug;
        return (StompServer &) *this;
      } // debug

      // ### SocketController pure virtuals ###
      void onConnect(const openframe::Connection *);
      void onDisconnect(const openframe::Connection *);
      void onRead(const openframe::Peer *);
      bool onPeerWake(const openframe::Peer *);
      const string::size_type onWrite(const openframe::Peer *, std::string &);

      // ### Stats Pure Virtuals ###
      virtual void onDescribeStats();
      virtual void onDestroyStats();


      // ### Queue Members ###
      void enqueue_queue(StompMessage *);
      void enqueue_queue(mesgList_t &);
      void exqueue_queues();
      void clear_queues();
      void dequeue_queue(StompMessage *);
      void enqueue_topic(StompMessage *);
      void exqueue_topics();
      void dequeue_topic(StompMessage *);

    protected:
      // ### Protected Members ###
      bool _process_peers(peers_t &peers);
      bool _process_peer(StompPeer *peer);
      void _process(StompPeer *, StompFrame *);
      void _process_connect(StompPeer *, StompFrame *);
      void _process_begin(StompPeer *, StompFrame *);
      void _process_commit(StompPeer *, StompFrame *);
      void _process_abort(StompPeer *, StompFrame *);
      void _process_ack(StompPeer *, StompFrame *);
      void _process_nack(StompPeer *, StompFrame *);
      void _process_send(StompPeer *, StompFrame *);
      void _process_subscribe(StompPeer *, StompFrame *);
      void _process_unsubscribe(StompPeer *, StompFrame *);
      void _process_receipt_id(StompPeer *, StompFrame *);
      void _process_disconnect(StompPeer *, StompFrame *);
      bool _store_transaction(StompPeer *, StompFrame *);
      void _send_queues(StompPeer *);

      // ### Protected Variables ###
      topics_t _topics;
      queues_t _queues;

    private:
      void init_stats(const bool startup=false);
      void _initializeThreads();
      void _deinitializeThreads();

      peers_t _peers;
      openframe::OFLock _peers_l;
      bool _debug;
      time_t _intval_logstats;
      time_t _time_queue_expire;
      size_t _max_work;
      size_t _queue_byte_limit;
      ExchangeManager *_exch_manager;

      struct obj_stats_t {
        size_t num_peers;
        time_t last_report_peers;
      } _stats; // obj_stats_t
  }; // StompServer

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/
} // namespace stomp
#endif
