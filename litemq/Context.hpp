#ifndef LITEMQ_CONTEXT_HPP_
#define LITEMQ_CONTEXT_HPP_

#include <memory>
#include "DescriptorHandlerPoller.hpp"

namespace LiteMQ
{
  // Context
  class Context: public Gears::CompositeActiveObject
  {
    friend class Connection;

  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    Context(
      const Gears::ActiveObjectCallback_var& active_object_callback,
      unsigned long threads_per_pool,
      unsigned long pools);

    virtual
    ~Context() throw() = default;

    virtual bool
    add_handler(const DescriptorHandler_var& desc_handler)
      throw(Exception);

  protected:
    DescriptorHandlerPollerPool_var descriptor_handler_poller_pool_;
  };

  typedef std::shared_ptr<Context> Context_var;
}

#endif /*LITEMQ_CONTEXT_HPP_*/
