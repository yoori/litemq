#ifndef EPOLLER_HPP_
#define EPOLLER_HPP_

#if __GNUC__ > 4 || \
  (__GNUC__ == 4 && (__GNUC_MINOR__ > 4))
#  include <atomic>
#else
#  include <cstdatomic>
#endif

#include <memory>
#include <unordered_map>
#include <list>

#include <gears/Exception.hpp>
#include <gears/Lock.hpp>
#include <gears/AtomicCounter.hpp>
#include <gears/CompositeActiveObject.hpp>

namespace LiteMQ
{
  //
  // DescriptorHandler
  //
  class DescriptorHandler
  {
  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    enum StateChange
    {
      CONTINUE_HANDLE = 0,
      STOP_READ_HANDLE = 1,
      START_READ_HANDLE = 2,
      STOP_WRITE_HANDLE = 4,
      START_WRITE_HANDLE = 8,
      STOP_PROCESSING = 16, // full stop of handlers processing loop
      STOP_HANDLE = STOP_READ_HANDLE | STOP_WRITE_HANDLE
    };

  public:
    virtual int
    fd() const throw() = 0;

    // return false if need stop loop
    virtual unsigned long
    read() throw(Exception) = 0;

    // return false if need stop loop
    virtual unsigned long
    write() throw(Exception) = 0;

    virtual void
    stopped() throw()
    {};
  };

  typedef std::shared_ptr<DescriptorHandler>
    DescriptorHandler_var;

  //
  // DescriptorHandlerOwner
  // interface that allow to assign ownership for DescriptorHandler
  //
  struct DescriptorHandlerOwner
  {
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    virtual
    ~DescriptorHandlerOwner() throw() = default;

    virtual bool
    add(const DescriptorHandler_var& desc_handler)
      throw(Exception) = 0;
  };

  typedef std::shared_ptr<DescriptorHandlerOwner> DescriptorHandlerOwner_var;

  //
  // DescriptorHandlerPoller
  //
  class DescriptorHandlerPoller:
    public std::enable_shared_from_this<DescriptorHandlerPoller>,
    public Gears::CompositeActiveObject,
    public DescriptorHandlerOwner
  {
  public:
    typedef DescriptorHandlerOwner::Exception Exception;

    class Proxy: public DescriptorHandlerOwner
    {
      friend class DescriptorHandlerPoller;

      typedef std::shared_ptr<DescriptorHandlerPoller> DescriptorHandlerPoller_var;

    public:
      bool
      add(const DescriptorHandler_var& desc_handler)
        throw(Exception);

      virtual
      ~Proxy() throw();

    protected:
      typedef Gears::RWLock SyncPolicy;

    protected:
      Proxy(DescriptorHandlerPoller* poller);

      void
      detach_() throw();

      DescriptorHandlerPoller_var
      lock_owner_() const throw();

    protected:
      mutable SyncPolicy lock_;
      DescriptorHandlerPoller* owner_;
    };

    typedef std::shared_ptr<Proxy> Proxy_var;

    class Impl;

    typedef std::shared_ptr<Impl> Impl_var;

  public:
    DescriptorHandlerPoller(
      const Gears::ActiveObjectCallback_var& callback,
      unsigned long threads,
      const Gears::Time& connection_clean_period = Gears::Time::ONE_SECOND)
      throw(Exception);

    virtual
    ~DescriptorHandlerPoller() throw();

    bool
    add(const DescriptorHandler_var& desc_handler)
      throw(Exception);

    // stop all process calls
    void
    stop() throw();

    Proxy_var
    proxy() const throw();

    void
    deactivate_object()
      throw(Gears::ActiveObject::Exception, Gears::Exception);

    void
    wait_object()
      throw(Gears::ActiveObject::Exception, Gears::Exception);

    static void
    set_non_blocking(int fd)
      throw(Exception);

    std::shared_ptr<DescriptorHandlerPoller>
    get_ptr()
    {
      return shared_from_this();
    }

  protected:
    typedef Gears::RWLock SyncPolicy;

    class StopPipeDescriptorHandler;

    typedef std::shared_ptr<StopPipeDescriptorHandler>
      StopPipeDescriptorHandler_var;

    struct DescriptorHandlerHolder:
      public std::enable_shared_from_this<DescriptorHandlerHolder>
    {
    public:
      DescriptorHandlerHolder(const DescriptorHandler_var& descriptor_handler)
        throw();

      virtual
      ~DescriptorHandlerHolder() throw()
      {
        descriptor_handler = DescriptorHandler_var();
        destroyed = true;
      }

      std::shared_ptr<DescriptorHandlerHolder>
      get_ptr()
      {
        return shared_from_this();
      }

      DescriptorHandler_var descriptor_handler;
      bool handle_read;
      bool handle_write;
      std::atomic<int> handle_in_progress;
      std::atomic<int> handling_finished;
      bool destroyed;
    };

    typedef std::shared_ptr<DescriptorHandlerHolder>
      DescriptorHandlerHolder_var;

    typedef std::unordered_map<DescriptorHandler*, DescriptorHandlerHolder_var>
      DescriptorHandlerMap;

    class DescriptorHandlerCleaner;

    typedef std::shared_ptr<DescriptorHandlerCleaner>
      DescriptorHandlerCleaner_var;

    friend class DescriptorHandlerCleaner;

  protected:
    void
    process_(unsigned long thread_i)
      throw();

    bool
    handle_(
      unsigned long thread_i,
      const DescriptorHandlerHolder_var& descriptor_handler_holder,
      uint32_t events);

    // epoll helpers
    void
    epoll_del_fd_(int fd)
      throw(Exception);

    void
    epoll_rearm_fd_(
      unsigned long thread_i,
      DescriptorHandlerHolder* descriptor_handler_holder)
      const throw(Exception);

    bool
    apply_state_modify_(
      unsigned long thread_i,
      DescriptorHandlerHolder* descriptor_handler_holder,
      unsigned long state_modify)
      const throw();

  protected:
    Gears::ActiveObjectCallback_var callback_;
    const int connection_clean_period_;

    Proxy_var proxy_;
    int epoll_fd_;
    StopPipeDescriptorHandler_var stop_handler_;
    DescriptorHandlerCleaner_var descriptor_handler_cleaner_;

    SyncPolicy handlers_lock_;
    DescriptorHandlerMap handlers_;
  };

  typedef std::shared_ptr<DescriptorHandlerPoller>
    DescriptorHandlerPoller_var;

  //
  // DescriptorHandlerPollerPool
  //
  class DescriptorHandlerPollerPool:
    public std::enable_shared_from_this<DescriptorHandlerPollerPool>,
    public Gears::CompositeActiveObject,
    public DescriptorHandlerOwner
  {
  public:
    typedef DescriptorHandlerOwner::Exception Exception;

    class Proxy: public DescriptorHandlerOwner
    {
      friend class DescriptorHandlerPollerPool;

      typedef std::shared_ptr<DescriptorHandlerPoller> DescriptorHandlerPoller_var;

    public:
      bool
      add(const DescriptorHandler_var& desc_handler)
        throw(Exception);

      virtual
      ~Proxy() throw();

    protected:
      typedef Gears::RWLock SyncPolicy;

    protected:
      Proxy(DescriptorHandlerPollerPool* poller);

      void
      detach_() throw();

      std::shared_ptr<DescriptorHandlerPollerPool>
      lock_owner_() const throw();

    protected:
      mutable SyncPolicy lock_;
      DescriptorHandlerPollerPool* owner_;
    };

    typedef std::shared_ptr<Proxy> Proxy_var;
    DescriptorHandlerPollerPool(
      const Gears::ActiveObjectCallback_var& callback,
      unsigned long threads_per_poller,
      unsigned long poller_num,
      const Gears::Time& connection_clean_period = Gears::Time::ONE_SECOND)
      throw(Exception);

    virtual
    ~DescriptorHandlerPollerPool() throw() = default;

    bool
    add(const DescriptorHandler_var& desc_handler)
      throw(Exception);

    Proxy_var
    proxy() const throw();

    std::shared_ptr<DescriptorHandlerPollerPool>
    get_ptr()
    {
      return shared_from_this();
    }

  private:
    typedef std::list<DescriptorHandlerPoller_var> DescriptorHandlerPollerList;
    typedef Gears::Mutex SyncPolicy;

  private:
    Proxy_var proxy_;

    DescriptorHandlerPollerList pollers_;
    SyncPolicy::Mutex poller_lock_;
    DescriptorHandlerPollerList::const_iterator poller_it_;
  };

  typedef std::shared_ptr<DescriptorHandlerPollerPool>
    DescriptorHandlerPollerPool_var;
}

#endif /*EPOLLER_HPP_*/
