#ifndef __MODULE_STOMPEXCEPTION_H
#define __MODULE_STOMPEXCEPTION_H

#include <sstream>
#include <exception>

#include <openframe/openframe.h>

namespace stomp {
  using openframe::StringTool;
  using openframe::stringify;
  using std::stringstream;
  using std::string;

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

class Stomp_Exception : public std::exception {
  public:
    Stomp_Exception() throw() : _message("an unknown exception occured")  { }
    Stomp_Exception(const string message) throw() {
      if (!message.length())
        _message = "An unknown message exception occured.";
      else
       _message = message;
    } // Stomp_Exception

    virtual ~Stomp_Exception() throw() { }
    virtual const char *what() const throw() { return _message.c_str(); }

    const char *message() const throw() { return _message.c_str(); }

  private:
    string _message;                    // Message of the exception error.
}; // class Stomp_Exception

class Frame_Exception : public Stomp_Exception {
  private:
    typedef Stomp_Exception super;
  public:
    Frame_Exception(const string message) throw() : super(message) { }
}; // class Frame_Exception

class StompIntegerOutOfRange_Exception : public Stomp_Exception {
  private:
    typedef Stomp_Exception super;
  public:
    StompIntegerOutOfRange_Exception(const string message) throw() : super(message) { }
    StompIntegerOutOfRange_Exception(const string &name, const int value, const int min, const int max) throw() : super("interger is out of range; name="+name+" value="+stringify<int>(value)+" min="+stringify<int>(min)+" max="+stringify<int>(max) ) { }
    StompIntegerOutOfRange_Exception(const string &name, const int value, const int max) throw() : super("interger is out of range; name="+name+" value="+stringify<int>(value)+" > max="+stringify<int>(max) ) { }
}; // class StompIntergerOutOfRange_Exception

class StompNotConnected_Exception : public Stomp_Exception {
  private:
    typedef Stomp_Exception super;
  public:
    StompNotConnected_Exception() throw() : super("stomp not connnected") { }
}; // class StompNotConnected_Exception

class StompDisconnected_Exception : public Stomp_Exception {
  private:
    typedef Stomp_Exception super;
  public:
    StompDisconnected_Exception() throw() : super("stomp disconnected") { }
}; // class StompDisconnected_Exception

class StompWrongLengthOfString_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompWrongLengthOfString_Exception(const string &name, const size_t len, const size_t min, const size_t max) throw() : super("string has wrong length; name="+name+" len="+stringify<int>(len)+" min="+stringify<int>(min)+" max="+stringify<int>(max) ) { }
    StompWrongLengthOfString_Exception(const string &name, const size_t len, const size_t max) throw() : super("string has wrong length; name="+name+" len="+stringify<int>(len)+" > max="+stringify<int>(max) ) { }
    StompWrongLengthOfString_Exception(const size_t len, const size_t a, const size_t b, const string &name) throw() : super("string has wrong length; name="+name+" len="+stringify<int>(len)+" must be "+stringify<int>(a)+" OR "+stringify<int>(b) ) { }
    StompWrongLengthOfString_Exception(const size_t len, const size_t a, const string &name) throw() : super("string has wrong length; name="+name+" len="+stringify<int>(len)+" != "+stringify<int>(a) ) { }
}; // class StompWrongLengthOfString_Exception

class StompPacketTooShort_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompPacketTooShort_Exception(const string &name, const size_t len, const size_t min) throw() : super("packet is too short (invalid); name="+name+" len="+stringify<int>(len)+" min="+stringify<int>(min) ) { }
}; // class StompPacketTooShort_Exception

class StompMissingFrameEnd_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompMissingFrameEnd_Exception() throw() : super("frame is missing end nul byte" ) { }
}; // class StompMessingFrameEnd_Exception

class StompInvalidHeader_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompInvalidHeader_Exception() throw() : super("frame has invalid header" ) { }
}; // class StompMessingFrameEnd_Exception

class StompNoSuchHeader_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompNoSuchHeader_Exception(const string &name) throw() : super("no such header; "+name) { }
}; // class StompNoSuchHeader_Exception

class StompValueNotSet_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompValueNotSet_Exception() throw() : super("value not set") { }
    StompValueNotSet_Exception(const string value_name) throw() : super("value not set for "+value_name) { }
}; // class StompValueNotSet_Exception

class StompUnknownCommandId_Exception : public Frame_Exception {
  private:
    typedef Frame_Exception super;
  public:
    StompUnknownCommandId_Exception(const uint32_t command_id) throw() : super("unknown command_id; id="+StringTool::char2hex(command_id) ) { }
}; // class StompUnknownCommandId_Exception

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

#define STOMP_TRY_RETHROW(a, b, c, d, e, f) \
try { a; } \
catch(b ex) { throw c(d, e, f); }

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

extern "C" {
} // extern

}
#endif
