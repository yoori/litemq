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
  // callback's used for implement case when need to switch sending from bad to good connection
  //
  class Connection
  {
  public:
    Connection(
      const Context_var& context,
      uint32_t ip_addr,
      unsigned int port,
      unsigned long max_out_size,
      unsigned long max_out_sockets);

    template<
      typename FailCallbackType,
      typename SuccessConnectCallbackType,
      typename DefaultRecvCallbackType>
    Connection(
      const Context_var& context,
      uint32_t ip_addr,
      unsigned int port,
      unsigned long max_out_size,
      unsigned long max_out_sockets,
      FailCallbackType fail_callback,
      SuccessConnectCallbackType success_connect_callback,
      DefaultRecvCallbackType default_recv_callback);

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
    class Callback;

    typedef std::unique_ptr<Callback> Callback_var;

    template<typename CallbackFunType>
    class CallbackImpl;

    class MessageCallback;

    typedef std::unique_ptr<MessageCallback> MessageCallback_var;

    template<typename CallbackFunType>
    class MessageCallbackImpl;

    struct State
    {
      // number of connections that ready for write (no waiting for write)
      std::atomic<int> ready_connections;

      // message queue for send
      MessageQueue message_queue;
    };

    typedef std::shared_ptr<State> State_var;

    // socket handler
    class SendDescriptorHandler;
    typedef std::shared_ptr<SendDescriptorHandler> SendDescriptorHandler_var;
    typedef std::deque<SendDescriptorHandler_var> SendDescriptorHandlerArray;

  private:
    bool
    wakeup_waiting_connection_();

    bool
    extend_connections_available_() const;

    void
    try_extend_connections_();

    void
    success_connect_();

  private:
    const Context_var context_;

    // connect endpoint
    const uint32_t ip_address_;
    const unsigned int port_;

    // config
    const unsigned long max_out_size_;
    const unsigned long max_out_sockets_;
    const Gears::Time MAX_DELAY_IN_POOL_;

    Callback_var connect_fail_callback_;
    Callback_var connect_success_callback_;
    MessageCallback_var default_recv_callback_;

    //
    State_var state_;

    Gears::Mutex waiting_handlers_lock_;
    SendDescriptorHandlerArray waiting_handlers_;
    std::atomic<int> opened_connections_;
  };

  typedef std::shared_ptr<Connection> Connection_var;
}

namespace LiteMQ
{
  class Connection::Callback
  {
  public:
    virtual void call() = 0;
  };

  template<typename CallbackFunType>
  class Connection::CallbackImpl: public Callback
  {
  public:
    CallbackImpl(CallbackFunType fun)
      : fun_(fun)
    {}

    virtual void call()
    {
      fun_();
    }

  protected:
    CallbackFunType fun_;
  };

  class Connection::MessageCallback
  {
  public:
    virtual void
    call(const void* data, unsigned long size) = 0;
  };

  template<typename MessageCallbackFunType>
  class Connection::MessageCallbackImpl: public MessageCallback
  {
  public:
    MessageCallbackImpl(MessageCallbackFunType fun)
      : fun_(fun)
    {}

    virtual void
    call(const void* data, unsigned long size)
    {
      fun_(data, size);
    }

  protected:
    MessageCallbackFunType fun_;
  };

  inline
  Connection::Connection(
    const Context_var& context,
    uint32_t ip_addr,
    unsigned int port,
    unsigned long max_out_size,
    unsigned long max_out_sockets)
    : context_(context),
      ip_address_(ip_addr),
      port_(port),
      max_out_size_(max_out_size),
      max_out_sockets_(max_out_sockets),
      opened_connections_(0)
  {}

  template<
    typename ConnectFailCallbackType,
    typename ConnectSuccessCallbackType,
    typename DefaultRecvCallbackType>
  Connection::Connection(
    const Context_var& context,
    uint32_t ip_addr,
    unsigned int port,
    unsigned long max_out_size,
    unsigned long max_out_sockets,
    ConnectFailCallbackType connect_fail_callback,
    ConnectSuccessCallbackType connect_success_callback,
    DefaultRecvCallbackType default_recv_callback)
    : context_(context),
      ip_address_(ip_addr),
      port_(port),
      max_out_size_(max_out_size),
      max_out_sockets_(max_out_sockets),
      connect_fail_callback_(new CallbackImpl<ConnectFailCallbackType>(connect_fail_callback)),
      connect_success_callback_(new CallbackImpl<ConnectSuccessCallbackType>(connect_success_callback)),
      default_recv_callback_(new MessageCallbackImpl<DefaultRecvCallbackType>(default_recv_callback))
  {}

  template<typename MessageType>
  bool
  Connection::send(MessageType message)
  {
    state_->message_queue.add(message);

    if(!wakeup_waiting_connection_() && extend_connections_available_())
    {
      // try increase number of connections if reached max delay in pool
      const Gears::Time queue_eldest_push_time =
        state_->message_queue.eldest_time();

      bool try_extend = false;

      if(queue_eldest_push_time != Gears::Time::ZERO)
      {
        const Gears::Time now = Gears::Time::get_time_of_day();

        if(now - queue_eldest_push_time > MAX_DELAY_IN_POOL_)
        {
          try_extend = true;
        }
      }

      if(try_extend)
      {
        try_extend_connections_();
      }
    }

    return true;
  }

}

#endif /*LITEMQ_CONNECTION_HPP_*/
