#ifndef ACCEPTORDESCRIPTORHANDLER_HPP_
#define ACCEPTORDESCRIPTORHANDLER_HPP_

#include "DescriptorHandlerPoller.hpp"

namespace LiteMQ
{
  class AcceptorDescriptorHandler: public DescriptorHandler
  {
  public:
    AcceptorDescriptorHandler(
      unsigned long port,
      const DescriptorHandlerOwner_var& poller_proxy)
      throw(Exception);

    virtual int
    fd() const throw();

    // return false if need stop loop
    virtual unsigned long
    read() throw(Exception);

    // return false if need stop loop
    virtual unsigned long
    write() throw(Exception);

    virtual void
    stopped() throw();

  protected:
    virtual
    ~AcceptorDescriptorHandler() throw();

    void
    accept_() throw(Exception);

    virtual DescriptorHandler_var
    create_descriptor_handler(int fd)
      throw() = 0;

  protected:
    DescriptorHandlerOwner_var proxy_;
    int fd_;
  };
}

#endif /*ACCEPTORDESCRIPTORHANDLER_HPP_*/
