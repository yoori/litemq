#include "Errno.hpp"
#include "OutputMemoryStream.hpp"

#include "ThreadRunner.hpp"

namespace Gears
{
  //
  // ThreadCallback class
  //

  void
  ThreadCallback::on_start() throw ()
  {}

  void
  ThreadCallback::on_stop() throw ()
  {}

  //
  // ThreadRunner::Options class
  //

  const size_t ThreadRunner::Options::DEFAULT_STACK_SIZE;

  ThreadRunner::Options::Options(
    size_t stack_size,
    const ThreadCallback_var& thread_callback) throw ()
    : stack_size(
        stack_size < PTHREAD_STACK_MIN ? DEFAULT_STACK_SIZE :
        stack_size),
      thread_callback(thread_callback)
  {}

  //
  // ThreadRunner::PThreadAttr class
  //

  ThreadRunner::PThreadAttr::PThreadAttr(size_t stack_size)
    throw (PosixException)
  {
    static const char* FUN = "ThreadRunner::PThreadAttr::PThreadAttr()";

    int res = ::pthread_attr_init(&attr_);
    if (res)
    {
      Gears::throw_errno_exception<PosixException>(res, FUN,
        "failed to initialize attribute");
    }
    res = ::pthread_attr_setstacksize(&attr_, stack_size);
    if (res)
    {
      Gears::throw_errno_exception<PosixException>(res, FUN,
        "tried to set stack size ", stack_size);
    }
  }

  ThreadRunner::PThreadAttr::~PThreadAttr() throw ()
  {
    ::pthread_attr_destroy(&attr_);
  }

  ThreadRunner::PThreadAttr::operator pthread_attr_t*() throw ()
  {
    return &attr_;
  }


  //
  // ThreadRunner class
  //

  ThreadRunner::ThreadRunner(
    const ThreadJob_var& job,
    unsigned number_of_jobs,
    const Options& options)
    throw (Gears::Exception, PosixException)
    : attr_(options.stack_size),
      thread_callback_(options.thread_callback),
      start_semaphore_(0),
      number_running_(0),
      number_of_jobs_(number_of_jobs)
  {
    jobs_.reserve(number_of_jobs_);

    for (unsigned i = 0; i < number_of_jobs_; i++)
    {
      jobs_.emplace_back(new JobInfo(this, job));
    }
  }

  ThreadRunner::~ThreadRunner() throw ()
  {
    try
    {
      wait_for_completion();
    }
    catch (...)
    {}
  }

  void
  ThreadRunner::thread_func_(ThreadJob& job) throw ()
  {
    start_semaphore_.acquire();
    start_semaphore_.release();

    if (number_running_ > 0)
    {
      if (thread_callback_)
      {
        thread_callback_->on_start();
      }

      job.work();

      if (thread_callback_)
      {
        thread_callback_->on_stop();
      }
    }
  }

  void*
  ThreadRunner::thread_func_(void* arg) throw ()
  {
    JobInfo* info = static_cast<JobInfo*>(arg);
    info->runner->thread_func_(*info->job);
    return 0;
  }

  void
  ThreadRunner::start_one_thread_() throw (PosixException)
  {
    static const char* FUN = "ThreadRunner::start_one_thread_()";

    const int RES = ::pthread_create(
      &jobs_[number_running_]->thread_id,
      attr_,
      thread_func_,
      jobs_[number_running_].get());

    if (RES)
    {
      Gears::throw_errno_exception<PosixException>(RES, FUN, "thread start");
    }

    number_running_++;
  }

  void
  ThreadRunner::wait_for_completion() throw (PosixException)
  {
    static const char* FUN = "ThreadRunner::wait_for_completion()";

    if (number_running_)
    {
      Gears::ErrorStream ostr;
      for (int i = 0; i < abs(number_running_); i++)
      {
        const int RES = ::pthread_join(jobs_[i]->thread_id, 0);

        if (RES)
        {
          char error[sizeof(PosixException)];
          Gears::ErrnoHelper::compose_safe(error, sizeof(error), RES,
            FUN, "join failure");
          ostr << error << "\n";
        }
      }

      start_semaphore_.acquire();
      number_running_ = 0;

      const Gears::SubString& str = ostr.str();
      if (str.size())
      {
        throw PosixException(str);
      }
    }
  }

  void
  ThreadRunner::start(unsigned to_start)
    throw (AlreadyStarted, PosixException, Gears::Exception)
  {
    static const char* FUN = "ThreadRunner::start()";

    if (number_running_)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": already started";
      throw AlreadyStarted(ostr.str());
    }

    if (!to_start || to_start > number_of_jobs_)
    {
      to_start = number_of_jobs_;
    }

    try
    {
      while (static_cast<unsigned>(number_running_) < to_start)
      {
        start_one_thread_();
      }
    }
    catch (const Gears::Exception&)
    {
      number_running_ = -number_running_;
      start_semaphore_.release();
      try
      {
        wait_for_completion();
      }
      catch (const Gears::Exception& ex)
      {
        abort();
      }
      throw;
    }

    start_semaphore_.release();
  }

  void
  ThreadRunner::start_one() throw (AlreadyStarted, PosixException)
  {
    static const char* FUN = "ThreadRunner::start_one()";

    if (static_cast<unsigned>(number_running_) == number_of_jobs_)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": all threads are already started";
      throw AlreadyStarted(ostr.str());
    }

    start_one_thread_();
  }
}
