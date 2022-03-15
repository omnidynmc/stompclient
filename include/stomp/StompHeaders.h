#ifndef LIBSTOMP_STOMPHEADERS_H
#define LIBSTOMP_STOMPHEADERS_H

#include <string>

#include <openframe/openframe.h>

#include "StompHeader.h"
#include "Stomp_Exception.h"

namespace stomp {

/**************************************************************************
 ** General Defines                                                      **
 **************************************************************************/

/**************************************************************************
 ** Structures                                                           **
 **************************************************************************/

  class StompHeaders : public openframe::Refcount, public stompHeader_t {
    public:
      StompHeaders();
      StompHeaders(const std::string &name, const std::string &value);
      virtual ~StompHeaders();

      // ### Public Members ###
      StompHeaders &add_header(const std::string &name, const std::string &value);
      StompHeaders &replace_header(const std::string &name, const std::string &value);
      StompHeaders &replace_header(StompHeader *header);
      StompHeaders &copy_headers_from(const StompHeaders *frame);
      StompHeaders &remove_header(const std::string &name);
      const std::string get_header(const std::string &name);
      const std::string get_header(const std::string &name, const std::string &def);
      const bool get_header(const std::string &name, StompHeader *&header);
      const bool is_header(const std::string &name);
      const std::string headersToString() const;

    protected:
      const stompHeaderSize_t headers(ByteData &buf);
      const std::string headers() const;

      // ### Protected Variables ###
      bool _requires_resp;

    private:
  }; // class StompHeaders

  std::ostream &operator<<(std::ostream &ss, const StompHeaders *headers);

/**************************************************************************
 ** Macro's                                                              **
 **************************************************************************/

/**************************************************************************
 ** Proto types                                                          **
 **************************************************************************/

} // namespace stomp
#endif
