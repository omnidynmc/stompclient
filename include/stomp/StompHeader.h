#ifndef LIBSTOMP_STOMPHEADER_H
#define LIBSTOMP_STOMPHEADER_H

#include <string>
#include <map>

#include <openframe/openframe.h>

#include "Stomp_Exception.h"

namespace stomp {
  using openframe::ByteData;
  using openframe::noCaseCompare;
  using std::string;
  using std::map;

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompHeader : public openframe::Refcount {
    public:
      StompHeader(const string &name, const string &value);
      StompHeader(ByteData buf);
      virtual ~StompHeader();

      inline const string name() const { return _name; }
      inline const string value() const { return _value; }

      virtual const string toString() const;
      virtual const string compile() const;

    protected:
      string _name;
      string _value;
    private:
  }; // class StompHeader

  typedef map<string, StompHeader *, noCaseCompare> stompHeader_t;
  typedef stompHeader_t::iterator stompHeader_itr;
  typedef stompHeader_t::const_iterator stompHeader_citr;
  typedef stompHeader_t::size_type stompHeaderSize_t;

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
