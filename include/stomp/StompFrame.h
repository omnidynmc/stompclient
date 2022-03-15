#ifndef LIBSTOMP_STOMPFRAME_H
#define LIBSTOMP_STOMPFRAME_H

#include <string>

#include <openframe/openframe.h>

#include "StompHeaders.h"
#include "Stomp_Exception.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompFrame : public StompHeaders {
    public:
      enum commandEnum {
        commandConnect		= 0,
        commandStomp		= 1,
        commandConnected	= 2,
        commandSend		= 3,
        commandSubscribe	= 4,
        commandUnsubscribe	= 5,
        commandBegin		= 6,
        commandCommit		= 7,
        commandAck		= 8,
        commandDisconnect	= 9,
        commandMessage		= 10,
        commandReceipt		= 11,
        commandError		= 12,
        commandAbort		= 13,
        commandNack		= 14,
        commandUnknown		= 999
      };

      StompFrame(const string &command, const string &body="");
      virtual ~StompFrame();

      // ### Public Members ###
      inline const string command() const { return _command; }
      inline const string body() const { return _body; }
      inline const bool is_command(const string &command) const { return (StringTool::toUpper(_command) == command); }
      inline const bool is_command(const commandEnum type) const { return (type == _type); }
      inline const commandEnum type() const { return _type; }
      inline const bool requires_resp() const { return _requires_resp; }
      inline void requires_resp(const bool rr) { _requires_resp=rr; }
      inline const bool execute_transaction() const { return _execute_transaction; }
      inline void set_execute_transaction(const bool et) { _execute_transaction=et; }
      inline const time_t created() const { return _created; }
      inline const time_t updated() const { return _updated; }
      inline const time_t updated_since() const { return ( time(NULL) - _updated ); }

      virtual const size_t length() { return _body.length(); }
      virtual const string compile();
      virtual const string toString() const;

    protected:
      // ### Protected Variables ###
      bool _requires_resp;
      bool _execute_transaction;

    private:
      // ### Private Variables ###
      string _command;
      string _body;
      time_t _created;
      time_t _updated;
      commandEnum _type;

      stringstream _out;
  }; // class StompFrame

  std::ostream &operator<<(std::ostream &ss, const StompFrame *frame);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
