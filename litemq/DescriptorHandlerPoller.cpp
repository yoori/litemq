#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>

#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <gears/Errno.hpp>
#include <gears/DelegateActiveObject.hpp>

#include "DescriptorHandlerPoller.hpp"

namespace LiteMQ
{
  namespace
  {
    const bool TRACE_POLLER_ = false; //true;
    const char TRACE_TIME_FORMAT[] = "%F %T";

    void
    mem_barrier()
    {
      asm volatile ("" :: : "memory");
    }

    const uint32_t EPOLL_FLAGS = EPOLLRDHUP | EPOLLET | EPOLLONESHOT /*| EPOLLEXCLUSIVE*/;
  }

  /* stopping with using StopPipeDescriptorHandler
   * write to pipe stop message
   * StopPipeDescriptorHandler read it - stop loop, decrease threads_to_stop and write message again if threads_to_stop isn't zero
   */
  class DescriptorHandlerPoller::StopPipeDescriptorHandler: public DescriptorHandler
  {
  public:
    StopPipeDescriptorHandler()
      throw();

    virtual
    ~StopPipeDescriptorHandler() throw();

    virtual int
    fd() const throw();

    virtual unsigned long
    read() throw(Exception);

    virtual unsigned long
    write() throw(Exception);

    virtual void
    stopped() throw();

    void
    stop() throw();

  protected:
    void
    stop_() throw();

  protected:
    volatile sig_atomic_t stopped_;
    int read_fd_;
    int write_fd_;
  };

  // DescriptorHandlerCleaner
  // class that allow to clean collected DescriptorHandler's(remove reference) only when
  // all threads called release
  //
  // For epoll this garantee that handler will be freed only when
  // all epoll_wait calls finished
  //
  class DescriptorHandlerPoller::DescriptorHandlerCleaner
  {
  public:
    DescriptorHandlerCleaner(unsigned long threads)
      throw();

    virtual
    ~DescriptorHandlerCleaner() throw()
    {}

    void
    push(const DescriptorHandlerHolder_var& descriptor_handler_holder)
      throw();

    void
    release(unsigned long id)
      throw();

  protected:
    typedef Gears::Mutex SyncPolicy;
    typedef std::list<DescriptorHandlerHolder_var> ObjList;
    struct CopyableAtomicInt: public std::atomic<int>
    {
      CopyableAtomicInt(int i)
        : std::atomic<int>(0)
      {}

      CopyableAtomicInt(const CopyableAtomicInt& i)
        : std::atomic<int>(i.load())
      {}

      CopyableAtomicInt(CopyableAtomicInt&& i)
        : std::atomic<int>(i.load())
      {}

      CopyableAtomicInt&
      operator=(const CopyableAtomicInt&& i)
      {
        static_cast<std::atomic<int>&>(*this) = i.load();
        return *this;
      };
    };

  protected:
    std::vector<CopyableAtomicInt> release_array_;
    Gears::AtomicCounter unreleased_count_;

    // descriptors that will be freed when current release loop will be finished
    SyncPolicy free_lock_;
    ObjList free_;

    // descriptors for that will be started new release after finish current
    SyncPolicy free_candidates_lock_;
    ObjList free_candidates_;
  };

  // DescriptorHandlerPoller::DescriptorHandlerHolder impl
  DescriptorHandlerPoller::DescriptorHandlerHolder::DescriptorHandlerHolder(
    const DescriptorHandler_var& descriptor_handler_val)
    throw()
    : descriptor_handler(descriptor_handler_val),
      handle_read(true),
      handle_write(true),
      handle_in_progress(0),
      handling_finished(0)
  {
    assert(descriptor_handler_val);
  }

  // DescriptorHandlerPoller::StopPipeDescriptorHandler impl
  DescriptorHandlerPoller::StopPipeDescriptorHandler::StopPipeDescriptorHandler()
    throw()
    : stopped_(0)
  {
    static const char* FUN = "StopPipeDescriptorHandler::StopPipeDescriptorHandler()";

    int fds[2];

    if(::pipe(fds) < 0)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, "pipe() failed");
    }

    read_fd_ = fds[0];
    write_fd_ = fds[1]; // blocked mode

    set_non_blocking(read_fd_);
  }

  DescriptorHandlerPoller::StopPipeDescriptorHandler::~StopPipeDescriptorHandler()
    throw()
  {
    ::close(read_fd_);
    ::close(write_fd_);
  }

  int
  DescriptorHandlerPoller::StopPipeDescriptorHandler::fd() const
    throw()
  {
    return read_fd_;
  }

  unsigned long
  DescriptorHandlerPoller::StopPipeDescriptorHandler::read()
    throw(Exception)
  {
    static const char* FUN = "StopPipeDescriptorHandler::read()";

    if(stopped_)
    {
      return DescriptorHandler::STOP_PROCESSING;
    }

    uint32_t value;
    ssize_t read_res = ::read(read_fd_, &value, 4);

    if(read_res < 0)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK)
      {
        return DescriptorHandler::STOP_WRITE_HANDLE;
      }
      else
      {
        Gears::throw_errno_exception<Exception>(errno, FUN, "read() failed");
      }
    }

    assert(read_res > 0);

    // memory barrier
    mem_barrier();

    // trust that write to pipe for 4 bytes can't be blocked
    stop_();

    // other thread(if exists) will write
    return static_cast<unsigned long>(DescriptorHandler::STOP_PROCESSING) |
      DescriptorHandler::STOP_WRITE_HANDLE;
  }

  unsigned long
  DescriptorHandlerPoller::StopPipeDescriptorHandler::write()
    throw(Exception)
  {
    return DescriptorHandler::STOP_WRITE_HANDLE;
  }

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stopped()
    throw()
  {}

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stop()
    throw()
  {
    // synchroniously write stop message
    stopped_ = 1;

    mem_barrier();

    // write stop message
    stop_();
  }

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stop_()
    throw()
  {
    static const char* FUN = "StopPipeDescriptorHandler::stop_()";

    // write stop message
    uint32_t value = 0;
    ssize_t write_res = ::write(write_fd_, &value, 4);

    if(write_res < 0)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, "write() failed");
    }
  }

  // DescriptorHandlerPoller::DescriptorHandlerCleaner
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::DescriptorHandlerCleaner(unsigned long threads)
    throw()
    : release_array_(threads, CopyableAtomicInt(0)),
      unreleased_count_(threads)
  {}

  void
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::push(const DescriptorHandlerHolder_var& obj)
    throw()
  {
    ObjList obj_list;
    obj_list.push_back(obj);

    {
      SyncPolicy::WriteGuard lock(free_candidates_lock_);
      free_candidates_.splice(free_candidates_.end(), obj_list);

      if(TRACE_POLLER_)
      {
        std::ostringstream ostr;
        ostr << "CLEAN: add handler, size = " << free_candidates_.size() << std::endl;
        std::cerr << ostr.str() << std::endl;
      }
    }
  }

  void
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::release(unsigned long id)
    throw()
  {
    if(release_array_[id] == 0) // optimization : prevent excess atomic call
    {
      if(release_array_[id]++ == 0)
      {
        if(unreleased_count_-- == 1)
        {
          // if release loop finished
          // clear all descriptors and swap candidates with free descriptors

          for(auto it = release_array_.begin(); it != release_array_.end(); ++it)
          {
            if((*it)-- == 1)
            {
              unreleased_count_ += 1;
            }
          }

          // this block can be called concurrently
          ObjList clear_list;

          {
            // free_ = free_candidates_
            // free_candidates_ => empty
            SyncPolicy::WriteGuard lock1(free_candidates_lock_);
            SyncPolicy::WriteGuard lock2(free_lock_);

            free_.swap(clear_list);
            free_candidates_.swap(free_);
          }

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            ostr << "CLEAN: free handlers, size = " << clear_list.size() << std::endl;
            std::cerr << ostr.str() << std::endl;
          }
        }
      }
      // revert increase for consistency
      else if(release_array_[id]-- == 1)
      {
        // if some release appeared concurrently with free ignore it
        // unreleased_count_ should be consistent with release_array_
        unreleased_count_ += 1;
      }
    }
  }

  // DescriptorHandlerPoller::Proxy impl
  DescriptorHandlerPoller::Proxy::Proxy(
    DescriptorHandlerPoller* poller)
    : owner_(poller)
  {}

  DescriptorHandlerPoller::Proxy::~Proxy()
    throw()
  {}

  bool
  DescriptorHandlerPoller::Proxy::add(
    const DescriptorHandler_var& desc_handler)
    throw(Exception)
  {
    DescriptorHandlerPoller_var owner = lock_owner_();

    if(owner)
    {
      owner->add(desc_handler);
      return true;
    }

    return false;
  }

  DescriptorHandlerPoller_var
  DescriptorHandlerPoller::Proxy::lock_owner_() const throw()
  {
    SyncPolicy::ReadGuard lock(lock_);
    if(owner_)
    {
      return owner_->get_ptr();
    }
    return DescriptorHandlerPoller_var();
  }

  void
  DescriptorHandlerPoller::Proxy::detach_() throw()
  {
    SyncPolicy::WriteGuard lock(lock_);
    owner_ = 0;
  }

  // DescriptorHandlerPoller implementation
  bool
  DescriptorHandlerPoller::handle_(
    unsigned long thread_i,
    const DescriptorHandlerHolder_var& descriptor_handler_holder,
    uint32_t events)
  {
    //static const char* FUN = "DescriptorHandlerPoller::handle_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
      ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
        "> #" << thread_i <<
        ": process fd(" << descriptor_handler_holder->descriptor_handler->fd() << "), events = " << events <<
        " =>" << (events & EPOLLIN ? " EPOLLIN" : "") <<
        (events & EPOLLOUT ? " EPOLLOUT" : "") <<
        (events & EPOLLRDHUP ? " EPOLLRDHUP" : "") <<
        (events & EPOLLERR ? " EPOLLERR" : "") <<
        std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    bool stop_processing = false;

    // lock handling - only one thread can call handlers
    // cleaner garantee that object live
    //
    int prev_handle_in_progress =
      descriptor_handler_holder->handle_in_progress++;

    if(descriptor_handler_holder->handling_finished == 0 &&
      prev_handle_in_progress == 0
      // delegate handling to thread that already do this
      )
    {
      if(TRACE_POLLER_)
      {
        std::ostringstream ostr;
        Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
        ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
          "> #" << thread_i <<
          ": real process fd(" << descriptor_handler_holder->descriptor_handler->fd() << "): " <<
          events << "(in = " <<
          descriptor_handler_holder->handle_read << ", out = " <<
          descriptor_handler_holder->handle_write << ")" << std::endl;
        std::cerr << ostr.str() << std::endl;
      }

      DescriptorHandler* descriptor_handler =
        descriptor_handler_holder->descriptor_handler.get();

      do
      {
        assert(prev_handle_in_progress >= 0);

        // signal epoll continue handling
        // rearm with state flags, before handle
        epoll_rearm_fd_(thread_i, descriptor_handler_holder.get());

        // call all handlers - it should detect
        // all event types (EPOLLIN, EPOLLOUT, EPOLLRDHUP, EPOLLERR)
        unsigned long state_modify_on_read = 0;
        unsigned long state_modify_on_write = 0;

        if(descriptor_handler_holder->handle_read)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": to handle read" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          state_modify_on_read = descriptor_handler->read();
          stop_processing |= apply_state_modify_(
            thread_i,
            descriptor_handler_holder.get(),
            state_modify_on_read);

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": from handle read (in = " <<
              descriptor_handler_holder->handle_read << ", out = " <<
              descriptor_handler_holder->handle_write << ", sw = " <<
              (state_modify_on_read & DescriptorHandler::START_READ_HANDLE) << ")" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }
        }

        if(descriptor_handler_holder->handle_write)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": to handle write" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          state_modify_on_write = descriptor_handler->write();
          stop_processing |= apply_state_modify_(
            thread_i,
            descriptor_handler_holder.get(),
            state_modify_on_write);

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": from handle write (in = " <<
              descriptor_handler_holder->handle_read << ", out = " <<
              descriptor_handler_holder->handle_write << ")" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }
        }

        // TODO: rearm here once instead rearm in apply_state_modify_
        // TODO: don't close if not stopped
        if(!descriptor_handler_holder->handle_read &&
          !descriptor_handler_holder->handle_write)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": stop handle(" << descriptor_handler->fd() << "): " <<
              "state_modify_on_read = " << state_modify_on_read <<
              ", state_modify_on_write = " << state_modify_on_write <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          descriptor_handler_holder->handling_finished = 1;

          epoll_del_fd_(descriptor_handler->fd());

          descriptor_handler->stopped();

          // minimize size of descriptor_handler_holder for keep in cleaner
          descriptor_handler_holder->descriptor_handler = DescriptorHandler_var();

          // add to clean list
          descriptor_handler_cleaner_->push(descriptor_handler_holder);

          SyncPolicy::WriteGuard lock(handlers_lock_);
          handlers_.erase(descriptor_handler);
          break; // ignore other thread handles on stop
        }
      }
      // process handles appeared in other threads
      while((prev_handle_in_progress =
        descriptor_handler_holder->handle_in_progress--) > 1);
    }

    assert(prev_handle_in_progress >= 0);

    // release current thread for cleaner
    descriptor_handler_cleaner_->release(thread_i);

    return stop_processing;
  }

  void
  DescriptorHandlerPoller::process_(unsigned long thread_i)
    throw()
  {
    static const char* FUN = "DescriptorHandlerPoller::process_()";

    const int MAX_EVENTS = 10000;

    epoll_event events[MAX_EVENTS];

    try
    {
      while(true)
      {
        int n = ::epoll_wait(epoll_fd_, events, MAX_EVENTS, connection_clean_period_);

        if(n < 0)
        {
          if(errno != EINTR)
          {
            Gears::throw_errno_exception<Exception>(errno, FUN, "epoll_wait() failed");
          }
          else
          {
            n = 0;
          }
        }

        bool stop = false;

        for(int i = 0; i < n; ++i)
        {
          // handle
          DescriptorHandlerHolder* descriptor_handler_holder =
            static_cast<DescriptorHandlerHolder*>(events[i].data.ptr);

          stop |= handle_(thread_i, descriptor_handler_holder->get_ptr(), events[i].events);
        }

        if(stop)
        {
          return;
        }

        // make release only after processing for decrease processing latency
        // release can call much handlers destroying
        descriptor_handler_cleaner_->release(thread_i);
      }
    }
    catch(const Gears::Exception& ex)
    {
      //std::cerr << ex.what() << std::endl;

      Gears::ErrorStream ostr;
      ostr << FUN << ": caught Gears::Exception: " << ex.what();
      callback_->critical(Gears::SubString(ostr.str()));
    }
  }

  void
  DescriptorHandlerPoller::epoll_del_fd_(int fd)
    throw(Exception)
  {
    static const char* FUN = "DescriptorHandlerPoller::epoll_del_fd_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      Gears::ExtendedTime now = Gears::Time::get_time_of_day().get_gm_time();
      ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
        "> epoll_del_fd_(" << fd << ")" << std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    epoll_event ev;

    if(::epoll_ctl(
         epoll_fd_,
         EPOLL_CTL_DEL,
         fd,
         &ev) == -1)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl(EPOLL_CTL_DEL) failed");
    }
  }

  void
  DescriptorHandlerPoller::epoll_rearm_fd_(
    unsigned long thread_i,
    DescriptorHandlerHolder* descriptor_handler_holder)
    const throw(Exception)
  {
    static const char* FUN = "DescriptorHandlerPoller::epoll_rearm_fd_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      ostr << Gears::Time::get_time_of_day().get_gm_time().format(TRACE_TIME_FORMAT) << "> #" << thread_i <<
        ": rearm fd(" <<
        descriptor_handler_holder->descriptor_handler->fd() << ")"
        ", handle_read = " << descriptor_handler_holder->handle_read <<
        ", handle_write = " << descriptor_handler_holder->handle_write <<
        std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    epoll_event ev;
    ev.events = EPOLL_FLAGS |
      (descriptor_handler_holder->handle_read ? EPOLLIN : 0) |
      (descriptor_handler_holder->handle_write ? EPOLLOUT : 0);
    ev.data.ptr = descriptor_handler_holder;

    if(::epoll_ctl(
         epoll_fd_,
         EPOLL_CTL_MOD,
         descriptor_handler_holder->descriptor_handler->fd(),
         &ev) == -1)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl() failed");
    }
  }

  bool
  DescriptorHandlerPoller::apply_state_modify_(
    unsigned long thread_i,
    DescriptorHandlerHolder* descriptor_handler_holder,
    unsigned long state_modify)
    const throw()
  {
    bool rearm = false;
    bool res_handle_read = descriptor_handler_holder->handle_read;
    bool res_handle_write = descriptor_handler_holder->handle_write;

    if(state_modify & DescriptorHandler::START_READ_HANDLE)
    {
      res_handle_read = true;
    }
    else if(state_modify & DescriptorHandler::STOP_READ_HANDLE)
    {
      res_handle_read = false;
    }

    if(state_modify & DescriptorHandler::START_WRITE_HANDLE)
    {
      res_handle_write = true;
    }
    else if(state_modify & DescriptorHandler::STOP_WRITE_HANDLE)
    {
      res_handle_write = false;
    }

    // rearm if processing stopped (other threads can use rearm)
    rearm = (res_handle_write != descriptor_handler_holder->handle_write) ||
      (res_handle_read != descriptor_handler_holder->handle_read);

    descriptor_handler_holder->handle_read = res_handle_read;
    descriptor_handler_holder->handle_write = res_handle_write;

    if(rearm)
    {
      epoll_rearm_fd_(thread_i, descriptor_handler_holder);
    }

    return state_modify & DescriptorHandler::STOP_PROCESSING;
  }

  // DescriptorHandlerPoller impl
  DescriptorHandlerPoller::DescriptorHandlerPoller(
    const Gears::ActiveObjectCallback_var& callback,
    unsigned long threads,
    const Gears::Time& connection_clean_period)
    throw(Exception)
    : callback_(callback),
      connection_clean_period_(connection_clean_period.microseconds() / 1000)
  {
    proxy_ = Proxy_var(new Proxy(this));

    descriptor_handler_cleaner_ = DescriptorHandlerCleaner_var(new DescriptorHandlerCleaner(threads));

    epoll_fd_ = ::epoll_create1(0);

    stop_handler_ = StopPipeDescriptorHandler_var(new StopPipeDescriptorHandler());

    add(stop_handler_);

    for(unsigned long thread_i = 0; thread_i < threads; ++thread_i)
    {
      Gears::DelegateActiveObject_var worker =
        Gears::make_delegate_active_object(
          std::bind(&DescriptorHandlerPoller::process_, this, thread_i),
          callback,
          1);

      add_child_object(worker);
    }
  }

  DescriptorHandlerPoller::~DescriptorHandlerPoller() throw()
  {
    proxy_->detach_();

    for(auto it = handlers_.begin(); it != handlers_.end(); ++it)
    {
      epoll_event ev;
      ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, it->first->fd(), &ev);
    }

    handlers_.clear();

    ::close(epoll_fd_);
  }

  DescriptorHandlerPoller::Proxy_var
  DescriptorHandlerPoller::proxy() const throw()
  {
    return proxy_;
  }

  bool
  DescriptorHandlerPoller::add(const DescriptorHandler_var& desc_handler)
    throw(Exception)
  {
    static const char* FUN = "DescriptorHandlerPoller::add()";

    bool inserted;

    DescriptorHandlerHolder_var holder = DescriptorHandlerHolder_var(
      new DescriptorHandlerHolder(desc_handler));

    {
      SyncPolicy::WriteGuard lock(handlers_lock_);
      inserted = handlers_.insert(std::make_pair(desc_handler.get(), holder)).second;
    }

    if(inserted)
    {
      epoll_event ev;
      ev.events = EPOLL_FLAGS | EPOLLIN | EPOLLOUT;
      ev.data.ptr = holder.get();

      if(TRACE_POLLER_)
      {
        std::cerr << "epoll_add_fd_(" << desc_handler->fd() << ")" << std::endl;
      }

      if(::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, desc_handler->fd(), &ev) == -1)
      {
        Gears::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl() failed");
      }
    }

    return true;
  }

  void
  DescriptorHandlerPoller::deactivate_object()
    throw(Gears::ActiveObject::Exception, Gears::Exception)
  {
    stop_handler_->stop();

    Gears::CompositeActiveObject::deactivate_object();
  }

  void
  DescriptorHandlerPoller::wait_object()
    throw(Gears::ActiveObject::Exception, Gears::Exception)
  {
    // wait when all threads will be finished
    Gears::CompositeActiveObject::wait_object();
  }

  void
  DescriptorHandlerPoller::set_non_blocking(int fd)
    throw(Exception)
  {
    static const char* FUN = "DescriptorHandlerPoller::set_non_blocking()";

    int flags = fcntl(fd, F_GETFL, 0);

    if(flags < 0)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, "fcntl() failed");
    }

    if(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      Gears::throw_errno_exception<Exception>(
        errno,
        FUN,
        "fcntl() for enable non blocking failed");
    }
  }

  // DescriptorHandlerPollerPool impl
  DescriptorHandlerPollerPool::DescriptorHandlerPollerPool(
    const Gears::ActiveObjectCallback_var& callback,
    unsigned long threads_per_poller,
    unsigned long poller_num,
    const Gears::Time& connection_clean_period)
    throw(Exception)
  {
    proxy_ = Proxy_var(new Proxy(this));

    for(unsigned long i = 0; i < poller_num; ++i)
    {
      DescriptorHandlerPoller_var poller(
        new DescriptorHandlerPoller(
          callback,
          threads_per_poller,
          connection_clean_period));

      pollers_.push_back(poller);
      add_child_object(poller);
    }

    poller_it_ = pollers_.begin();
  }

  DescriptorHandlerPollerPool::Proxy_var
  DescriptorHandlerPollerPool::proxy() const throw()
  {
    return proxy_;
  }

  bool
  DescriptorHandlerPollerPool::add(const DescriptorHandler_var& desc_handler)
    throw(Exception)
  {
    DescriptorHandlerPollerList::const_iterator poller_it;

    {
      SyncPolicy::WriteGuard lock(poller_lock_);
      poller_it = poller_it_++;
      if(poller_it_ == pollers_.end())
      {
        poller_it_ = pollers_.begin();
      }
    }

    return (*poller_it)->add(desc_handler);
  }

  // DescriptorHandlerPollerPool::Proxy impl
  DescriptorHandlerPollerPool::Proxy::Proxy(
    DescriptorHandlerPollerPool* poller)
    : owner_(poller)
  {}

  DescriptorHandlerPollerPool::Proxy::~Proxy()
    throw()
  {}

  bool
  DescriptorHandlerPollerPool::Proxy::add(
    const DescriptorHandler_var& desc_handler)
    throw(Exception)
  {
    DescriptorHandlerPollerPool_var owner = lock_owner_();

    if(owner)
    {
      owner->add(desc_handler);
      return true;
    }

    return false;
  }

  DescriptorHandlerPollerPool_var
  DescriptorHandlerPollerPool::Proxy::lock_owner_() const throw()
  {
    SyncPolicy::ReadGuard lock(lock_);
    if(owner_)
    {
      return owner_->get_ptr();
    }
    return DescriptorHandlerPollerPool_var();
  }

  void
  DescriptorHandlerPollerPool::Proxy::detach_() throw()
  {
    SyncPolicy::WriteGuard lock(lock_);
    owner_ = 0;
  }
}


