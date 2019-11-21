// Generics/ActiveObject.cpp

#include <iostream>

#include "ActiveObject.hpp"

namespace Gears
{
  //
  // SimpleActiveObject class
  //

  SimpleActiveObject::~SimpleActiveObject() throw ()
  {
    if (state_ != AS_NOT_ACTIVE)
    {
      std::cerr << "SimpleActiveObject is not deactivated" << std::endl;
    }
  }

  void
  SimpleActiveObject::activate_object()
    throw (AlreadyActive, Exception, Gears::Exception)
  {
    static const char* FUN = "SimpleActiveObject::activate_object()";

    {
      Condition::Guard guard(cond_);
      if (state_ == AS_NOT_ACTIVE)
      {
        activate_object_();
        state_ = AS_ACTIVE;
        return;
      }
    }

    Gears::ErrorStream ostr;
    ostr << FUN << "already active";
    throw AlreadyActive(ostr.str());
  }

  void
  SimpleActiveObject::deactivate_object() throw (Exception, Gears::Exception)
  {
    Condition::Guard guard(cond_);

    if (state_ != AS_ACTIVE)
    {
      return;
    }
    state_ = AS_DEACTIVATING;
    cond_.broadcast();
    try
    {
      deactivate_object_();
    }
    catch (...)
    {
      state_ = AS_ACTIVE;
      throw;
    }
  }

  void
  SimpleActiveObject::wait_object() throw (Exception, Gears::Exception)
  {
    {
      Condition::Guard guard(cond_);
      while (state_ == AS_ACTIVE || wait_more_())
      {
        guard.wait();
      }
    }

    wait_object_();

    Condition::Guard guard(cond_);
    if (state_ == AS_DEACTIVATING)
    {
      state_ = AS_NOT_ACTIVE;
    }
  }

  bool
  SimpleActiveObject::active() throw (Gears::Exception)
  {
    return state_ == AS_ACTIVE;
  }

  void
  SimpleActiveObject::activate_object_() throw (Exception, Gears::Exception)
  {}

  void
  SimpleActiveObject::deactivate_object_() throw (Exception, Gears::Exception)
  {}

  bool
  SimpleActiveObject::wait_more_() throw (Exception, Gears::Exception)
  {
    return false;
  }

  void
  SimpleActiveObject::wait_object_() throw (Exception, Gears::Exception)
  {}

  //
  // ActiveObjectCommonImpl class
  //

  ActiveObjectCommonImpl::ActiveObjectCommonImpl(
    const SingleJob_var& job,
    unsigned threads_number,
    size_t stack_size,
    unsigned start_threads)
    throw (InvalidArgument)
    : SINGLE_JOB_(job),
      thread_runner_(
        job,
        threads_number,
        ThreadRunner::Options(stack_size, job->callback())),
      start_threads_(start_threads),
      work_mutex_(job->mutex()),
      active_state_(AS_NOT_ACTIVE)
  {
    static const char* FUN = "ActiveObjectCommonImpl::ActiveObjectCommonImpl()";

    if (!threads_number)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": threads_number == 0";
      throw InvalidArgument(ostr.str());
    }
  }

  ActiveObjectCommonImpl::~ActiveObjectCommonImpl() throw ()
  {
    static const char* FUN = "ActiveObjectCommonImpl::~ActiveObjectCommonImpl()";

    try
    {
      Gears::ErrorStream ostr;
      bool error = false;

      {
        Mutex::WriteGuard guard(work_mutex_);

        if (active_state_ == AS_ACTIVE)
        {
          ostr << FUN << ": wasn't deactivated.";
          error = true;
        }

        if (active_state_ != AS_NOT_ACTIVE)
        {
          if (error)
          {
            ostr << std::endl;
          }
          ostr << FUN << ": didn't wait for deactivation, still active.";
          error = true;
        }
      }

      if (error)
      {
        {
          Mutex::WriteGuard guard(work_mutex_);
          SINGLE_JOB_->make_terminate();
        }

        thread_runner_.wait_for_completion();

        {
          Mutex::WriteGuard guard(work_mutex_);
          SINGLE_JOB_->terminated();
        }

        ActiveObjectCallback_var callback = SINGLE_JOB_->callback();
        if (!callback)
        {
          std::cerr << ostr.str() << std::endl;
        }
        else
        {
          callback->warning(ostr.str());
        }
      }
    }
    catch (const Gears::Exception& ex)
    {
      try
      {
        std::cerr << FUN << ": Gears::Exception: " << ex.what() << std::endl;
      }
      catch (...)
      {
        // Nothing we can do
      }
    }
  }

  void
  ActiveObjectCommonImpl::activate_object()
    throw (AlreadyActive, Exception, Gears::Exception)
  {
    static const char* FUN = "ActiveObjectCommonImpl::activate_object()";

    Mutex::WriteGuard guard(work_mutex_);

    if (active_state_ != AS_NOT_ACTIVE)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": still active";
      throw ActiveObject::AlreadyActive(ostr.str());
    }

    try
    {
      active_state_ = AS_ACTIVE;
      thread_runner_.start(start_threads_);
    }
    catch (const Gears::Exception& ex)
    {
      active_state_ = AS_NOT_ACTIVE;

      Gears::ErrorStream ostr;
      ostr << FUN << ": start failure: " << ex.what();
      throw Exception(ostr.str());
    }

    SINGLE_JOB_->started(start_threads_);
  }

  void
  ActiveObjectCommonImpl::wait_object() throw (Exception, Gears::Exception)
  {
    static const char* FUN = "ActiveObjectCommonImpl::wait_object()";

    Mutex::WriteGuard termination_guard(termination_mutex_);

    if (active_state_ != AS_NOT_ACTIVE)
    {
      try
      {
        thread_runner_.wait_for_completion();
        SINGLE_JOB_->terminated();
      }
      catch (const Gears::Exception& ex)
      {
        Gears::ErrorStream ostr;
        ostr << FUN << "waiting failure: " << ex.what();
        throw Exception(ostr.str());
      }
    }

    Mutex::WriteGuard guard(work_mutex_);

    if(active_state_ == AS_DEACTIVATING)
    {
      active_state_ = AS_NOT_ACTIVE;
    }
  }

  void
  ActiveObjectCommonImpl::deactivate_object()
    throw (Exception, Gears::Exception)
  {
    Mutex::WriteGuard guard(work_mutex_);

    if(active_state_ == AS_ACTIVE)
    {
      active_state_ = AS_DEACTIVATING;
      SINGLE_JOB_->make_terminate();
    }
  }

  bool
  ActiveObjectCommonImpl::active()
    throw (Gears::Exception)
  {
    return active_state_ == AS_ACTIVE;
  }
} // namespace Gears
