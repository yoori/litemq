#ifndef LITEMQ_CONTEXT_HPP_
#define LITEMQ_CONTEXT_HPP_

#include <memory>
#include "DescriptorHandlerPoller.hpp"

namespace LiteMQ
{
  // Context
  class Context: public Gears::CompositeActiveObject
  {
  public:
    Context(const Gears::ActiveObjectCallback_var& active_object_callback);

    virtual
    ~Context() throw() = default;

  protected:
    DescriptorHandlerPollerPool_var descriptor_handler_poller_pool_;
  };

  typedef std::shared_ptr<Context> Context_var;
}

#endif /*LITEMQ_CONTEXT_HPP_*/
