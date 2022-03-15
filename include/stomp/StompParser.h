#ifndef LIBSTOMP_STOMPPARSER_H
#define LIBSTOMP_STOMPPARSER_H

#include <sstream>
#include <set>
#include <map>
#include <queue>
#include <string>

#include <openframe/openframe.h>
#include <openstats/openstats.h>

#include "StompServer.h"
#include "StompFrame.h"
#include "StompMessage.h"
#include "Subscription.h"
#include "TransactionManager.h"

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

  class StompParser_Exception : public openframe::OpenFrame_Exception {
    public:
      StompParser_Exception(const std::string message) throw() : openframe::OpenFrame_Exception(message) { };
  }; // class StompParser_Exception

  class StompParser : public TransactionManager,
                      public openstats::StatsClient_Interface,
                      virtual public openframe::Refcount {
    public:
      static const time_t kDefaultStatsInterval;

      StompParser();
      virtual ~StompParser();

      /**********************
       ** Type Definitions **
       **********************/
      typedef std::queue<StompFrame *> frameQueue_t;
      typedef frameQueue_t::size_type frameQueueSize_t;

      typedef std::map<std::string, Subscription *> subscriptions_t;
      typedef subscriptions_t::iterator subscriptions_itr;
      typedef subscriptions_t::const_iterator subscriptions_citr;
      typedef subscriptions_t::size_type subscriptions_st;

      typedef std::set<Subscription *> binds_t;
      typedef binds_t::iterator binds_itr;
      typedef binds_t::const_iterator binds_citr;
      typedef binds_t::size_type binds_st;

      enum stompStageEnum {
        stompStageCommand 	= 0,
        stompStageHeaders 	= 1,
        stompStageBodyLen	= 2,
        stompStageBodyNul	= 3,
        stompStageWaitForNul	= 4
      };

      // ### Public Members - Bindings ###
      bool bind(Subscription *);
      bool unbind(Subscription *);
      void unbind_all();
      bool is_bound(const std::string &);
      const bool is_match(const std::string &, Subscription *&);
      Subscription *find_bind(const std::string &id);
      const bool received_ack(const std::string &, const std::string &);
      const bool received_nack(const std::string &, const std::string &);

      bool store_subscription(Subscription *sub);
      bool forget_subscription(Subscription *sub);
      Subscription *find_subscription(const std::string &id);

      size_t receive(const char *buf, const size_t len);
      const string::size_type transmit(string &);
      void reset();
      bool process();

      // ### Heart-beat members ###
      void enable_heart_beat(const time_t sender_supported, const time_t sender_wants);
      void try_heart_beat();
      bool is_heart_beat_time();
      bool is_heart_beat_timeout();

      virtual void onRecoverableError(StompFrame *frame) = 0;
      virtual void onFatalError(StompFrame *frame) = 0;
      virtual void onBind(Subscription *sub) = 0;
      virtual void onUnbind(Subscription *sub) = 0;

      const bool dequeue(StompFrame *&ret) {
        if (!_frameQ.size()) return false;
        ret = _frameQ.front();
        _frameQ.pop();
        return true;
      } // dequeue

      // ### Protocol Commands ###
      const size_t send_frame(StompFrame *);
      const size_t send_error(const std::string &, const std::string &body="");
      const size_t disconnect_with_error(const std::string &, const std::string &body="");
      virtual bool next_frame(StompFrame *&frame);
      bool connect(const std::string &, const std::string &, StompHeaders *headers=NULL);
      bool connected(const std::string &, StompHeaders *headers=NULL);
      bool send(const std::string &, const std::string &, StompHeaders *headers=NULL);
      bool subscribe(const std::string &destination, const std::string &subsription,
                     StompHeaders *headers=NULL);
      bool unsubscribe(const std::string &, StompHeaders *headers=NULL);
      bool begin(const std::string &, StompHeaders *headers=NULL);
      bool commit(const std::string &, StompHeaders *headers=NULL);
      bool ack(const std::string &message_id, const std::string &subscription, StompHeaders *headers=NULL);
      bool nack(const std::string &message_id, const std::string &subscription, StompHeaders *headers=NULL);
      bool abort(const std::string &, StompHeaders *headers=NULL);
      bool message(const std::string &, const std::string &, const std::string &, StompHeaders *headers=NULL);
      bool receipt(const std::string &, StompHeaders *headers=NULL);

      bool commit_transaction(const std::string &key);

      // ### Stats Pure Virtuals ###
      virtual void onDescribeStats();
      virtual void onDestroyStats();

    protected:
      bool _process_command();
      bool _process_headers();
      bool _process_body_len();
      bool _process_body_nul();
      bool _process_waitfor_nul();
      void _process_reset();
      void _clear_frameq();
      std::string _uniq_id;

      typedef struct {
        std::string command;
        std::string body;
        stompHeader_t headers;
        size_t content_length;
      } stompStage_t;

      // ### Protected Members ###
      void _init();
      void init_stats(const bool startup=false);
      bool try_stats();
      virtual const string::size_type _write(const std::string &);

      // ### Protected Variables ###
      frameQueue_t _sentQ;
      binds_t _binds;
      subscriptions_t _subscriptions;

      openframe::OFLock _in_l;
      openframe::OFLock _out_l;
      openframe::StreamParser _in;
      std::string _out;

      frameQueue_t _frameQ;
      stompStageEnum _stage;
      stompStage_t _stagedFrame;

      struct obj_stats_t {
        size_t num_frames_in;
        size_t num_frames_out;
        size_t num_bytes_in;
        size_t num_bytes_out;
        time_t last_report_at;
        time_t created_at;
      } _stats;

      struct heart_beat_t {
        bool enabled;
        time_t ping_out;
        time_t ping_in;
        double last_ping_out;
        double last_ping_in;
      } _heart_beat;

    private:
  }; // StompParser

  std::ostream &operator<<(std::ostream &ss, const StompParser::stompStageEnum stage);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
