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
#include <stomp/Stomp.h>
#include <stomp/StompFrame.h>
#include <stomp/StompClient.h>
#include <stomp/StompHeader.h>

std::string g_last_message_id;
size_t g_num_messages = 0;
std::string g_sub_id = "1";

bool try_ack(stomp::Stomp *stomp, const bool force=false) {
  static time_t last_ack = time(NULL);

  if (!g_num_messages) return false;
  bool ok = g_num_messages >= 2048 || last_ack < time(NULL) - 2 || force;
  if (!ok) return false;

  ok = stomp->ack(g_last_message_id, g_sub_id);

  time_t diff = time(NULL) - last_ack;

  std::cout << "Sending ack for "
            << g_num_messages
            << " message" << (g_num_messages == 1 ? "" : "s")
            << " after " << diff << " second" << (diff == 1 ? "" : "s")
            << " " << (force ? "(force)" : "")
            << std::endl;

  g_num_messages = 0;
  last_ack = time(NULL);

  return true;
} // try_ack

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cerr << "ERROR missing host argument" << std::endl;
    exit(1);
  } // if

  std::string hosts = argv[1];

  stomp::Stomp *stomp;
  try {
    stomp = new stomp::Stomp(hosts, "user", "pass");
  } // try
  catch(stomp::Stomp_Exception ex) {
    std::cout << "ERROR: " << ex.message() << std::endl;
    return 0;
  } // catch

  while(1) {
    std::cout << "Connecting to stomp server" << std::endl;

    bool ok = stomp->subscribe("/queue/feeds.*", "1");
    if (!ok) {
      std::cout << "not connected, retry in 2 seconds" << std::endl;
      sleep(2);
      continue;
    } // if

    while(1) {
      stomp::StompFrame *frame;
      ok = false;

      try_ack(stomp);

      try {
        ok = stomp->next_frame(frame);
      } // try
      catch(stomp::Stomp_Exception ex) {
        std::cout << "ERROR: " << ex.message() << std::endl;
        break;
      } // catch

      if (!ok) {
        std::cout << "no work, sleeping 2 seconds" << std::endl;
        sleep(2);
        continue;
      } // if

//      std::cout << frame << std::endl;

      if (frame->is_command(stomp::StompFrame::commandMessage) ) {
        g_last_message_id = frame->get_header("message-id");
        ++g_num_messages;
      } // if

    } // while

    std::cout << "not connected, reconnecting in 2 seconds" << std::endl;
    sleep(2);
  } // while

  return 0;
} // main
