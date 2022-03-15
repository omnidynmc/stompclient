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

class Parser : public stomp::StompParser {
  public:
    Parser() { }
    virtual ~Parser() { }

    virtual void onRecoverableError(stomp::StompFrame *frame) {
      std::cout << "ERROR  : " << frame->body() << std::endl;
      std::cout << "MESSAGE: " << frame->get_header("message", "no message") << std::endl;
      assert(false);
    } // onRecoverableError

    virtual void onFatalError(stomp::StompFrame *frame) {
      std::cout << "ERROR  : " << frame->body() << std::endl;
      std::cout << "MESSAGE: " << frame->get_header("message", "no message") << std::endl;
      assert(false);
    } // onFatalError

    virtual void onBind(stomp::Subscription *sub) { }
    virtual void onUnbind(stomp::Subscription *sub) { }

  protected:
  private:
}; // class Parser

int main(int argc, char **argv) {
  Parser *parser = new Parser();

  const time_t stats_intval = 5;
  const time_t sleep_intval = 2000;

  srand( time(NULL) );

  time_t last_stats = time(NULL);
  size_t num_in = 0;
  size_t num_bytes = 0;

/*
  parser->receive("MESSAGE\n", 8);
std::cout << "1" << std::endl;
  parser->process();
  parser->receive("Content-Lengt", 13);
std::cout << "2" << std::endl;
  parser->process();
  parser->receive("h:190\n", 6);
std::cout << "3" << std::endl;
  parser->process();
  parser->receive("destina", 7);
std::cout << "4" << std::endl;
  parser->process();
  parser->receive("tion:/", 6);
std::cout << "5" << std::endl;
  parser->process();
  parser->receive("topic/this.is.a.test\n", 21);
std::cout << "6" << std::endl;
  parser->process();
  parser->receive("\n", 1);
std::cout << "7" << std::endl;
  parser->process();
  parser->receive("=cC?[q_DpV(|2a<mJAuZ9Mf", 23);
std::cout << "8" << std::endl;
  parser->process();

  exit(0);

SENT(MESSAGE
Content-Lengt)
SENT(h:190
destina)
SENT(tion:/)
SENT(topic/this.is.a.test

=cC?[q_DpV(|2a<mJAuZ9Mf)
*/

  for(size_t i=0; i < 100; i++) {
    std::string out;
    for(size_t i=0; i < 100; i++) {
      std::string buf = openframe::StringTool::randstr(1, 200);
      stomp::StompFrame *frame = new stomp::StompFrame("MESSAGE", buf);
      frame->add_header("destination", "/topic/this.is.a.test");
      out += frame->compile();
      frame->release();
    } // for

    while(out.length()) {
      size_t len = rand() % 100;
      if (len > out.length()) len = out.length();
//std::cout << "SENT(" << out.substr(0, len) << ")" << std::endl;
      parser->receive(out.data(), len);
      parser->process();
      out.erase(0, len);
    } // while

    while(true) {
      stomp::StompFrame *frame;
      bool ok = parser->next_frame(frame);
      if (!ok) break;
      std::cout << frame << std::endl;
    } // while
  } // while

  delete parser;
  exit(0);
} // main
