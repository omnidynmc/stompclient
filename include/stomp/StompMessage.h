#ifndef LIBSTOMP_STOMPMESSAGE_H
#define LIBSTOMP_STOMPMESSAGE_H

#include <string>
#include <deque>

#include <openframe/openframe.h>

#include "StompFrame.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompMessage : public StompFrame {
    public:
      StompMessage(const std::string &destination, const std::string &body, const time_t inactivity_timeout=0);
      StompMessage(const std::string &destination, const std::string &transaction, const std::string &body, const time_t inactivity_timeout=0);
      virtual ~StompMessage();

      // ### Public Members ###
      inline const std::string destination() const { return _destination; }
      inline const std::string transaction() const { return _transaction; }
      inline const std::string id() const { return _id; }
      inline const bool is_id(const std::string &id) { return (id == _id); }
      inline const std::string body() const { return _body; }
      const std::string toString() const;

      inline time_t inactivity_timeout() const { return _inactivity_timeout; }
      inline time_t created() const { return _created; }
      inline time_t last_activity() const { return _last_activity; }
      inline bool is_inactive() const { return ( _inactivity_timeout && _last_activity < (time(NULL) - _inactivity_timeout) ); }
      inline time_t sent() const { return _sent; }
      inline bool is_sent() const { return (_sent > 0); }
      inline void mark_sent() { _sent = time(NULL); }
      inline void mark_unsent() { _sent = 0; }
      inline void inc_attempt() { _num_attempts++; }
      inline unsigned int num_attempts() const { return _num_attempts; }
      static const std::string create_uuid();

    protected:

    private:
      std::string _destination;
      std::string _transaction;
      std::string _id;
      std::string _body;
      time_t _inactivity_timeout;
      time_t _created;
      time_t _last_activity;
      time_t _sent;
      unsigned int _num_attempts;
  }; // StompMessage

  typedef std::deque<StompMessage *> mesgList_t;
  typedef mesgList_t::iterator mesgList_itr;
  typedef mesgList_t::const_iterator mesgList_citr;
  typedef mesgList_t::size_type mesgList_st;

  std::ostream &operator<<(std::ostream &ss, const StompMessage *smesg);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
