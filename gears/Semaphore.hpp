#ifndef SYNC_SEMAPHORE_HPP
#define SYNC_SEMAPHORE_HPP

#include <semaphore.h>

#include "Errno.hpp"
#include "Uncopyable.hpp"
#include "Time.hpp"
#include "Condition.hpp"

namespace Gears
{
  /**
   * @class Semaphore
   *
   * @brief Classic Semaphore semantic implementation (acquire/release)
   *
   * Now, system semaphore has no advantage before conditional variable imlementations
   * and class implemented with using conditional variable (futex on most platforms)
   */
  class Semaphore: private Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    explicit
    Semaphore(int count) throw (Exception);

    ~Semaphore() throw ();

    void
    acquire() throw (Exception);

    bool
    try_acquire() throw (Exception);

    bool
    timed_acquire(const Time* time, bool time_is_relative = false)
      throw (Exception);

    void
    release() throw (Exception);

    int
    value() throw (Exception);

  private:
    Condition condition_lock_;
    int count_;
  };
}

namespace Gears
{
  inline
  Semaphore::Semaphore(int count) throw (Exception)
    : count_(count)
  {}

  inline
  Semaphore::~Semaphore() throw ()
  {}

  inline
  void
  Semaphore::acquire() throw (Exception)
  {
    Condition::Guard lock(condition_lock_);

    while(count_ <= 0)
    {
      lock.wait();
    }

    --count_;
  }

  inline
  bool
  Semaphore::try_acquire() throw (Exception)
  {
    Condition::Guard lock(condition_lock_);

    if(count_ > 0)
    {
      --count_;
      return true;
    }

    return false;
  }

  inline
  bool
  Semaphore::timed_acquire(
    const Time* time,
    bool time_is_relative)
    throw (Exception)
  {
    if(!time)
    {
      acquire();
      return true;
    }

    Time real_time(time_is_relative ?
      Time::get_time_of_day() + *time : *time);

    Condition::Guard lock(condition_lock_);

    while(count_ <= 0)
    {
      if(!lock.timed_wait(&real_time))
      {
        return false;
      }
    }

    --count_;
    return true;
  }

  inline
  void
  Semaphore::release() throw (Exception)
  {
    {
      Condition::Guard lock(condition_lock_);
      ++count_;
    }

    condition_lock_.signal();
  }

  inline
  int
  Semaphore::value() throw (Exception)
  {
    Condition::Guard lock(condition_lock_);
    return count_;
  }
}

#endif
