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
#include <signal.h>

#include <openframe/openframe.h>
#include <stomp/StompServer.h>
#include <stomp/StompParser.h>
#include <stomp/StompFrame.h>

bool is_done = false;

void sighandler(int sig) {
  std::cout << "### SIGINT Caught, shutting down"<< std::endl;
  is_done = true;
} // sighandler

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "ERROR: missing port" << std::endl;
    return 1;
  } // if

  int port = atoi(argv[1]);

  signal(SIGINT, sighandler);

  stomp::StompServer *sserv = new stomp::StompServer(port, 100);

  sserv->intval_logstats(300);
  sserv->set_queue_byte_limit(10000);
  sserv->time_queue_expire(900);
  sserv->start();

  while(!is_done) {
    bool did_work = sserv->run();
    if (!did_work) sleep(1);
  } // while

  sserv->stop();

  delete sserv;

  return 0;
} // main
