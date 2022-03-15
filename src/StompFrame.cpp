#include "config.h"

#include <set>
#include <string>
#include <cassert>
#include <map>
#include <new>
#include <iostream>

#include <openframe/openframe.h>

#include "StompFrame.h"
#include "Stomp_Exception.h"

namespace stomp {
  using std::stringstream;
  using std::string;
  using std::cout;
  using std::endl;

/**************************************************************************
 ** StompFrame Class                                                     **
 **************************************************************************/
  StompFrame::StompFrame(const string &command, const string &body)
             : _requires_resp(true),
               _execute_transaction(false),
               _command(command),
               _body(body),
               _created( time(NULL) ),
               _updated( time(NULL) ) {
    if (is_command("CONNECT"))
      _type = commandConnect;
    else if (is_command("STOMP"))
      _type = commandStomp;
    else if (is_command("CONNECTED"))
      _type = commandConnected;
    else if (is_command("SEND"))
      _type = commandSend;
    else if (is_command("SUBSCRIBE"))
      _type = commandSubscribe;
    else if (is_command("UNSUBSCRIBE"))
      _type = commandUnsubscribe;
    else if (is_command("BEGIN"))
      _type = commandBegin;
    else if (is_command("COMMIT"))
      _type = commandCommit;
    else if (is_command("ABORT"))
      _type = commandAbort;
    else if (is_command("ACK"))
      _type = commandAck;
    else if (is_command("NACK"))
      _type = commandNack;
    else if (is_command("DISCONNECT"))
      _type = commandDisconnect;
    else if (is_command("MESSAGE"))
      _type = commandMessage;
    else if (is_command("RECEIPT"))
      _type = commandReceipt;
    else if (is_command("ERROR"))
      _type = commandError;
    else
      _type = commandUnknown;
  } // StompFrame::StompFrame

  StompFrame::~StompFrame() {
  } // StompFrame::~StompFrame

  const string StompFrame::compile() {
    stringstream out;
    replace_header("Content-Length", stringify<size_t>(_body.length() ) );
    out.str("");
    out << _command
        << endl
        << headers()
        << endl
        << _body
        << '\0' << endl;
    return out.str();
  } // StompFrame::compile

  const string StompFrame::toString() const {
    stringstream out;
    out << "StompFrame "
        << "command=" << _command
        << ",body=" << _body;
//        << headersToString();

     return out.str();
  } // StompFrame::toString

 std::ostream &operator<<(std::ostream &ss, const StompFrame *frame) {
    ss << frame->toString();
    return ss;
  } // operator<<
} // namespace stomp
