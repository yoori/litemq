#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_

namespace ServerGears
{
  // Application send message and set Callback (functor)
  struct MessageBase
  {
    virtual
    const void* data() const throw() = 0;

    virtual
    unsigned long size() const throw() = 0;
  };

  typedef std::shared_ptr<MessageBase> MessageBase_var;

  //
  class ReqResConnection
  {
    // how to close sockets
  public:
    ReqResConnection(const DescriptorHandlerPoller_var& poller);

    void
    send(const MessageBase_var& message) throw();

    
  protected:
    // handlers
  protected:
    unsigned long request_id_;
  };

  typedef std::shared_ptr<Connection> Connection_var;
}

#endif /*CONNECTION_HPP_*/
