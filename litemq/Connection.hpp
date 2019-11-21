#ifndef LITEMQ_CONNECTION_HPP_
#define LITEMQ_CONNECTION_HPP_

#include <memory>

#include "MessageQueue.hpp"
#include "Context.hpp"

namespace LiteMQ
{
  // class Connection
  //
  // message life cycle
  // if waiting_handlers non empty push message to first,
  // otherwise push message to queue
  //
  // handlers try to pop messages from queue when writing available
  // if here no messages in queue push handler to waiting_handlers
  //
  class Connection
  {
  public:
    template<
      typename FailCallbackType,
      typename SuccessConnectCallbackType,
      typename DefaultRecvCallbackType>
    Connection(
      const Context_var& context,
      FailCallbackType fail_callback,
      SuccessConnectCallbackType success_connect_callback,
      DefaultRecvCallbackType default_recv_callback,
      unsigned long max_out_size,
      unsigned long max_out_sockets);

    template<typename MessageType>
    bool
    send(MessageType message);

    // return false if out queue full
    // timeout_callback called if message not sent in defined time interval
    template<
      typename MessageType,
      typename TimeoutCallbackType>
    bool
    send(
      MessageType message,
      TimeoutCallbackType timeout_callback,
      const Gears::Time& timeout);

    // return false if out queue full
    // timeout_callback called if message not sent or response not received
    // in defined time interval or
    template<
      typename MessageType,
      typename ResponseCallbackType,
      typename TimeoutCallbackType>
    bool
    send_request(MessageType message,
      ResponseCallbackType response_callback,
      TimeoutCallbackType timeout_callback,
      const Gears::Time& timeout);

  private:
    class State
    {
      // number of connections that ready for write (no waiting for write)
      std::atomic<int> ready_connections;

      // message queue for send
      MessageQueue message_queue;
    };

    typedef std::shared_ptr<State> State_var;

    // socket handler
    class Handler;
    typedef std::shared_ptr<Handler> Handler_var;
    typedef std::vector<DescriptorHandler_var> DescriptorHandlerArray;

  private:
    Context_var context_;
    DescriptorHandlerArray waiting_handlers_;
  };
}

#endif /*LITEMQ_CONNECTION_HPP_*/
