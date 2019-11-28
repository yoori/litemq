#include "Context.hpp"

namespace LiteMQ
{
  Context::Context(
    const Gears::ActiveObjectCallback_var& active_object_callback,
    unsigned long threads_per_pool,
    unsigned long pools)
    : descriptor_handler_poller_pool_(new DescriptorHandlerPollerPool(
        active_object_callback,
        threads_per_pool,
        pools,
        Gears::Time::ONE_SECOND))
  {
    add_child_object(descriptor_handler_poller_pool_);
  }

  bool
  Context::add_handler(const DescriptorHandler_var& desc_handler)
    throw(Exception)
  {
    static const char* FUN = "Context::add_handler()";

    try
    {
      return descriptor_handler_poller_pool_->add(desc_handler);
    }
    catch(const Gears::Exception& ex)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": caught Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }
}

