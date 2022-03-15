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
#include <stomp/StompParser.h>
#include <stomp/StompFrame.h>

#include "Push.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cerr << "ERROR missing host argument" << std::endl;
    exit(1);
  } // if

  std::string host = argv[1];

  stomp::Push *push = new stomp::Push(host, "feedtest", "feedtest");
  push->set_connect_read_timeout(0);
  push->start();

  srand( time(NULL) );

  while(1) {
    std::string randstr = openframe::StringTool::randstr(10, 10);

    if ( !push->is_ready() ) {
      std::cout << "not connected, sleeping" << std::endl;
      sleep(5);
      continue;
    } // if

    push->post("/queue/feeds.test", randstr);

    usleep(1000);
  } // while

  exit(0);
} // main
