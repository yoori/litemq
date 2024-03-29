#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <unordered_map>
#include <set>

#include <gears/StringManip.hpp>
#include <gears/OutputMemoryStream.hpp>
#include <gears/StreamLogger.hpp>
#include <gears/ActiveObjectCallback.hpp>

//#include <gears/Tokenizer.hpp>

#include <litemq/AcceptorDescriptorHandler.hpp>
#include <litemq/DescriptorHandlerPoller.hpp>
#include <litemq/MessageQueue.hpp>
#include <litemq/Connection.hpp>
#include <litemq/Message.hpp>

void
conn_fail_callback()
{}

void
conn_success_connect_callback()
{}

void
default_recv_callback(const void*, unsigned long)
{}

int
main(int argc, char** argv)
{
  Gears::Logger_var logger(
    new Gears::OStream::Logger(
      Gears::OStream::Config(std::cerr)));

  Gears::ActiveObjectCallback_var callback(
    new Gears::ActiveObjectCallbackImpl(logger));

  LiteMQ::Context_var context(new LiteMQ::Context(callback, 4, 12));

  struct sockaddr_in sa;
  ::inet_pton(AF_INET, "192.0.2.33", &(sa.sin_addr));

  LiteMQ::Connection_var connection(
    new LiteMQ::Connection(
      context,
      sa.sin_addr.s_addr,
      11111,
      10000,
      10,
      conn_fail_callback,
      conn_success_connect_callback,
      default_recv_callback));

  context->activate_object();

  std::vector<unsigned char> msg(100, 'a');
  LiteMQ::MessageBuffer msg_buf(std::move(msg));
  connection->send(std::move(msg_buf));

  context->wait_object();

  return 0;
}
