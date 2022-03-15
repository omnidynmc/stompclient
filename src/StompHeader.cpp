#include "config.h"

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

#include "StompHeader.h"

namespace stomp {
  using namespace openframe;
  using namespace std;

/**************************************************************************
 ** StompFrame Class                                                     **
 **************************************************************************/
  StompHeader::StompHeader(const string &name, const string &value) : _name(name), _value(value) { }
  StompHeader::StompHeader(ByteData buf) {
    if (buf.length() < 1 || buf.find(':') == string::npos)
      throw StompInvalidHeader_Exception();

    StringToken st;
    st.setDelimiter(':');

    st = buf;
    if (st.size() < 1)
      throw StompInvalidHeader_Exception();

    _name = st[0];
    _value = (st.size() > 1 ? StringTool::trim( st.trail(1) ) : "");
  } // StompHeader::StompHeader
  StompHeader::~StompHeader() { }

  const string StompHeader::toString() const {
    stringstream out;
    out << "Header "
        << "name=" << _name
        << ",value=" << StringTool::safe(_value);
    return out.str();
  } // StompHeader::toString

  const string StompHeader::compile() const {
    stringstream out;
    out << _name << ":" << _value;
    return out.str();
  } // StompHeader::compile
} // namespace stomp
