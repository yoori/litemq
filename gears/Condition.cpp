// Condition.cpp

#include "Errno.hpp"
#include "Condition.hpp"

namespace Gears
{
  //
  // class Conditional
  //

  Conditional::~Conditional() throw ()
  {
    pthread_cond_destroy(&cond_);
  }

  void
  Conditional::wait(pthread_mutex_t& mutex)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "Conditional::wait()";

    // When we call pthread_cond_wait mutex_ should be locked,
    // otherwise - U.B.
    // pthread_cond_wait releases the mutex_ and blocks the thread
    // until another thread calls signal().
    // Mutex is used to protect data required for condition calculation.
    const int RES = pthread_cond_wait(&cond_, &mutex);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        "Failed to wait on condition");
    }
  }

  bool
  Conditional::timed_wait(pthread_mutex_t& mutex,
    const Gears::Time* time, bool time_is_relative)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "Conditional::timed_wait()";

    if (!time)
    {
      wait(mutex);
      return true;
      //We don't reach INFINITY time and cannot reach timeout
    }
    Gears::Time real_time(time_is_relative ?
      Gears::Time::get_time_of_day() + *time : *time);
    // When we call pthread_cond_wait mutex_ should be locked,
    // otherwise - U.B.
    // pthread_cond_wait releases the mutex_ and blocks the thread
    // until another thread calls signal().
    // Mutex is used to protect data required for condition calculation.
    const timespec RESTRICT =
      { real_time.tv_sec, real_time.tv_usec * 1000 };
    const int RES = pthread_cond_timedwait(&cond_, &mutex, &RESTRICT);
    if (RES == ETIMEDOUT)
    {
      return false;
    }
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        "Failed to wait on condition");
    }
    return true;
  }

  void
  Conditional::signal() throw (Exception, Gears::Exception)
  {
    static const char* FUN = "Conditional::signal()";

    // When we call pthread_cond_signal mutex_ should be locked,
    // otherwise - U.B.
    const int RES = pthread_cond_signal(&cond_);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        "Failed to signal condition");
    }
  }

  void
  Conditional::broadcast() throw (Exception, Gears::Exception)
  {
    static const char* FUN = "Conditional::broadcast()";

    // When we call pthread_cond_blodcast mutex_ should be locked,
    // otherwise - U.B.
    const int RES = pthread_cond_broadcast(&cond_);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        "Failed to broadcast condition");
    }
  }

  //
  // class Condition
  //

  void
  Condition::wait() throw (Exception, Gears::Exception)
  {
    Conditional::wait(this->mutex_i());
  }

  bool
  Condition::timed_wait(const Gears::Time* time, bool time_is_relative)
    throw (Exception, Gears::Exception)
  {
    return Conditional::timed_wait(this->mutex_i(), time, time_is_relative);
  }

  //
  // class Guard
  //

  Condition::Guard::Guard(Condition& condition)
    throw ()
    : WriteGuard(condition),
      conditional_(condition),
      mutex_(condition.mutex_i())
  {}

  void
  Condition::Guard::wait()
    throw (Conditional::Exception, Gears::Exception)
  {
    conditional_.wait(mutex_);
  }

  bool
  Condition::Guard::timed_wait(
    const Gears::Time* time,
    bool time_is_relative)
    throw (Conditional::Exception, Gears::Exception)
  {
    return conditional_.timed_wait(mutex_, time, time_is_relative);
  }
}
