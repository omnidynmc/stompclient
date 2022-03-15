#include "config.h"

#include <set>
#include <string>
#include <cassert>
#include <map>
#include <new>
#include <iostream>

#include <openframe/openframe.h>

#include "StompHeader.h"
#include "StompHeaders.h"
#include "Stomp_Exception.h"

namespace stomp {
/**************************************************************************
 ** StompHeaders Class                                                   **
 **************************************************************************/
  StompHeaders::StompHeaders() {
  } // StompHeaders::StompHeaders

  StompHeaders::StompHeaders(const std::string &name, const std::string &value) {
    add_header(name, value);
  } // StompHeaders::StompHeaders

  StompHeaders::~StompHeaders() {
    // free all of our headers
    for(stompHeader_itr itr = begin(); itr != end(); itr++) {
      StompHeader *header = itr->second;
      header->release();
    } // for
  } // StompHeaders::~StompHeaders

  StompHeaders &StompHeaders::add_header(const std::string &name, const std::string &value) {
    if (!is_header(name))
      insert( make_pair(name, new StompHeader(name, value)) );
    return dynamic_cast<StompHeaders &>(*this);
  } // StompHeaders::add_header

  StompHeaders &StompHeaders::replace_header(const std::string &name, const std::string &value) {
    if (is_header(name)) remove_header(name);
    insert( make_pair(name, new StompHeader(name, value)) );
    return dynamic_cast<StompHeaders &>(*this);
  } // StompHeaders::replace_header

  StompHeaders &StompHeaders::replace_header(StompHeader *header) {
    if (is_header( header->name() )) remove_header( header->name() );
    header->retain();
    insert( make_pair(header->name(), header) );
    return dynamic_cast<StompHeaders &>(*this);
  } // StompHeaders::replace_header

  StompHeaders &StompHeaders::remove_header(const std::string &name) {
      stompHeader_itr itr = find(name);
      if (itr == end()) throw StompNoSuchHeader_Exception(name);
      StompHeader *header = itr->second;
      header->release();
      erase(itr);
      return dynamic_cast<StompHeaders &>(*this);
  } // StompHeaders::remove_header

  StompHeaders &StompHeaders::copy_headers_from(const StompHeaders *headers) {
    for(stompHeader_citr citr = headers->begin(); citr != headers->end(); citr++) {
      StompHeader *header = citr->second;
      replace_header(header);
    } // for
    return dynamic_cast<StompHeaders &>(*this);
  } // StompHeaders::copy_headers_from

  const std::string StompHeaders::get_header(const std::string &name) {
    stompHeader_itr itr = find(name);
    if (itr == end()) throw StompNoSuchHeader_Exception(name);

    return itr->second->value();
  } // StompHeaders::get_header

  const std::string StompHeaders::get_header(const std::string &name, const std::string &def) {
    stompHeader_itr itr = find(name);
    if (itr == end()) return def;

    return itr->second->value();
  } // StompHeaders::get_header

  const bool StompHeaders::get_header(const std::string &name, StompHeader *&header) {
    stompHeader_itr itr = find(name);
    if (itr == end()) return false;
    header = itr->second;
    return true;
  } // StompHeaders::get_header

  const bool StompHeaders::is_header(const std::string &name) {
    StompHeader *header = NULL;
    return get_header(name, header);
  } // StompHeaders::is_header

  const std::string StompHeaders::headersToString() const {
    stompHeader_t::const_iterator itr;
    std::stringstream out;

    for(itr = begin(); itr != end(); itr++)
      out << itr->second->toString() << std::endl;

    return out.str();
  } // StompHeaders::headersToString

  // Private
  const stompHeaderSize_t StompHeaders::headers(ByteData &buf) {
    while(buf.nextLength() != 0) {
      std::string line;
      try {
        line = buf.nextLine();
      } // try
      catch(std::out_of_range ex) {
        throw StompInvalidHeader_Exception();
      } // catch

      if (line.length() < 1)
        break;

      openframe::StringToken st;
      st.setDelimiter(':');
      st = line;

      if (st.size() < 2)
        throw StompInvalidHeader_Exception();

      std::string name = st[0];
      std::string value = st.trail(1);

      StompHeader *header = new StompHeader(name, value);
      insert( std::make_pair(name, header) );
    } // while

    return size();
  } // StompHeaders::headers

  const std::string StompHeaders::headers() const {
    stompHeader_t::const_iterator itr;
    std::stringstream ret;

    for(itr = begin(); itr != end(); itr++) {
      StompHeader *header = itr->second;
      ret << header->compile() << std::endl;
    } // for
    return ret.str();
  } // StompHeaders::headers

 std::ostream &operator<<(std::ostream &ss, const StompHeaders *headers) {
    ss << headers->headersToString();
    return ss;
  } // operator<<
} // namespace stomp
