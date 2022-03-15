#include "config.h"

#include <string>
#include <cassert>
#include <list>
#include <map>
#include <new>
#include <iostream>
#include <sstream>

#include <ossp/uuid++.hh>

#include <openframe/openframe.h>

#include "StompMessage.h"

namespace stomp {

/**************************************************************************
 ** StompFrame Class                                                     **
 **************************************************************************/
  StompMessage::StompMessage(const string &destination, const string &body, const time_t inactivity_timeout)
               : StompFrame("MESSAGE", body),
                 _destination(destination),
                 _body(body),
                 _inactivity_timeout(inactivity_timeout),
                 _created( time(NULL) ),
                 _last_activity( time(NULL) ),
                 _sent(0),
                 _num_attempts(0) {
    _id = create_uuid();
    replace_header("message-id", _id );
    replace_header("destination", destination);
  } // StompMessage::StompMessage

  StompMessage::StompMessage(const string &destination, const string &transaction, const string &body, const time_t inactivity_timeout)
               : StompFrame("MESSAGE", body),
                 _destination(destination),
                 _transaction(transaction),
                 _body(body),
                 _inactivity_timeout(inactivity_timeout),
                 _created( time(NULL) ),
                 _last_activity( time(NULL) ),
                 _sent(0),
                 _num_attempts(0) {
    _id = create_uuid();
    replace_header("message-id", _id );
    replace_header("destination", destination);
    replace_header("transaction", transaction);
  } // StompMessage::StompMessage

  StompMessage::~StompMessage() {
  } // StompMessage::~StompMessage

  const string StompMessage::toString() const {
    stringstream out;
    out << "StompMessage destination=" << _destination
        << ",id=" << _id
        << ",transaction=" << _transaction
        << ",body=" << (_body.length() < 128 ? _body : _body.substr(0, 128)+"...")
        << ",inactivity_timeout=" << _inactivity_timeout;
    return out.str();
  } // StompMessage::toString

  const string StompMessage::create_uuid() {
    uuid id;
    id.make(UUID_MAKE_V1);
    char *str = id.string();
    string ret = string(str);
    free(str);
    return ret;
  } // StompMessage::create_uuid

 std::ostream &operator<<(std::ostream &ss, const StompMessage *smesg) {
    ss << smesg->toString();
    return ss;
  } // operator<<
} // namespace stomp
