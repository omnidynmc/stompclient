#include <cassert>
#include <exception>
#include <iostream>
#include <new>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openframe/openframe.h>

#include "StompParser.h"
#include "StompFrame.h"

#include "Feed.h"

stomp::Feed *_feed;
std::string _last_message_id;
size_t _num_messages = 0;

bool try_ack(const bool force=false) {
  static time_t _last_ack = time(NULL);

  if (!_num_messages) return false;
  bool ok = _num_messages >= 2048 || _last_ack < time(NULL) - 2 || force;
  if (!ok) return false;

  stomp::StompFrame *frame = new stomp::StompFrame("NACK");
  frame->add_header("subscription", _feed->sub_id() );
  frame->add_header("message-id", _last_message_id );
  size_t len = _feed->send_frame(frame);
  frame->release();

  time_t diff = time(NULL) - _last_ack;

  std::cout << "Sending nack for "
            << _num_messages
            << " message" << (_num_messages == 1 ? "" : "s")
            << " to " << _feed->dest_in()
            << " after " << diff << " second" << (diff == 1 ? "" : "s")
            << " " << (force ? "(force)" : "")
            << std::endl;

  _num_messages = 0;
  _last_ack = time(NULL);

  return true;
} // try_ack

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cerr << "ERROR missing host argument" << std::endl;
    exit(1);
  } // if

  std::string host = argv[1];

  _feed = new stomp::Feed(host, "feedtest", "feedtest", "/queue/feeds.*");
  _feed->set_connect_read_timeout(0);
  _feed->start();

  while(1) {
    bool didWork = false;
    size_t process_loops = 0;
//    if (process_loops) std::cout << "Process loops " << process_loops << std::endl;
    try_ack();

    size_t num_frames_in = 0;
    size_t num_bytes_in = 0;
    size_t frame_loops = 0;
    for(size_t i=0; i < 1024; i++) {
      stomp::StompFrame *frame;
      bool ok = _feed->next_frame(frame);
      if (!ok) break;
//      if (i == 2) std::cout << "Over 10" << std::endl;

      didWork |= ok;
      if (frame->is_command(stomp::StompFrame::commandMessage) ) {
        _last_message_id = frame->get_header("message-id");
        ++_num_messages;
      } // if

      ++frame_loops;
      frame->release();
    } // for

//    if (frame_loops) std::cout << "Frame loops " << frame_loops << std::endl;
    if (!didWork) usleep(1000);
  } // while

  exit(0);
} // main
