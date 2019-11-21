#ifndef GEARS_SYNC_CONDITION_HPP
#define GEARS_SYNC_CONDITION_HPP

#include "Lock.hpp"
#include "Time.hpp"

namespace Gears
{
  /**
   * @class Conditional
   *
   * @brief Conditional variable wrapper, which allows threads
   * to block until shared data changes state.
   *
   * A condition variable enables threads to atomically block and
   * test the condition under the protection of a mutual exclusion
   * lock (mutex) until the condition is satisfied.  That is,
   * the mutex must have been held by the thread before calling
   * wait or signal on the condition.  If the condition is false,
   * a thread blocks on a condition variable and atomically
   * releases the mutex that is waiting for the condition to
   * change.  If another thread changes the condition, it may wake
   * up waiting threads by signaling the associated condition
   * variable.  The waiting threads, upon awakening, reacquire the
   * mutex and re-evaluate the condition.
   */
  class Conditional : private Gears::Uncopyable
  {
  public:
    // Can be raised if system API errors occurred
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    /**
     * Constructor
     */
    constexpr
    Conditional() throw ();

    /**
     * Destructor
     */
    ~Conditional() throw ();

    // Lock accessors.

    /**
     * Block on condition.
     * @param mutex
     * Wait functions shall block on a condition variable. It
     * shall be called with mutex locked by the calling thread
     * or undefined behavior results.
     */
    void
    wait(pthread_mutex_t& mutex) throw (Exception, Gears::Exception);

    /**
     * Block on condition, or until absolute time-of-day has passed.
     * Wait functions shall block on a condition variable.
     * @param mutex Method shall be called with mutex locked by the
     * calling thread or undefined behavior results.
     * @param time pointer to absolute time or time interval in
     * dependency of third parameter. If pointer = 0 use blocking wait()
     * semantics.
     * This is useful if we choose time interval and sometime need
     * infinity waiting.
     * @param time_is_relative = true time parameter should be time interval.
     * Implementation add this time interval to current system time.
     * @return false if timeout.
     */
    bool
    timed_wait(pthread_mutex_t& mutex,
      const Gears::Time* time,
      bool time_is_relative = false)
      throw (Exception, Gears::Exception);

    /**
     * Signal one waiting thread. This method shall unblock at least one
     * of the threads that are blocked on Conditional
     * (if any threads are blocked on this).
     */
    void
    signal() throw (Exception, Gears::Exception);

    /**
     * Signal *all* waiting threads. This method shall unblock all threads
     * currently blocked on Conditional
     */
    void
    broadcast() throw (Exception, Gears::Exception);

  private:
    pthread_cond_t cond_;
  };

  /**
   * @class Condition
   *
   * @brief Condition is Conditional with mutex
   */
  class Condition:
    public virtual Conditional,
    public Gears::Mutex
  {
  public:
    /**
     * @class Guard
     *
     * @brief Guard is useful guard that locks associated with
     * Conditional mutex in constructor and unlock in destructor
     * And it will delegate calls to used Conditional while created.
     */
    class Guard: private Mutex::WriteGuard
    {
    public:
      /**
       * Constructor use Condition.
       * @param condition Lock internal_mutex of condition.
       * Methods calls will be delegate to condition.
       */
      explicit
      Guard(Condition& condition)
        throw ();

      /**
       * Destructor unlocks mutex that locked by constructor.
       */

      /**
       * Block on condition. Delegate call to conditional.
       */
      void
      wait() throw (Conditional::Exception, Gears::Exception);

      /**
       * Block on condition or until absolute time-of-day has passed.
       * Delegate call to conditional.
       * @param time pointer to absolute time or time interval in dependency
       * of second parameter. If pointer = 0 use blocking wait() semantics.
       * This is useful if we choose time interval and sometime need
       * infinity waiting.
       * @param time_is_relative if = true time parameter should be time
       * interval.
       * Implementation add this time interval to current system time.
       */
      bool
      timed_wait(const Gears::Time* time,
        bool time_is_relative = false)
        throw (Conditional::Exception, Gears::Exception);

    private:
      Conditional& conditional_;
      pthread_mutex_t& mutex_;
    };

    // Lock accessors.

    /**
     * Block on condition.
     * Wait functions shall block on a condition variable. It
     * shall be called when internal_mutex locked by the calling thread
     * or undefined behavior results.
     */
    void
    wait() throw (Exception, Gears::Exception);

    /**
     * Block on condition, or until absolute time-of-day has passed.
     * Wait functions shall block on a condition variable. It
     * shall be called with mutex locked by the calling thread
     * or undefined behavior results.
     * @param time pointer to absolute time or time interval in dependency
     * of second parameter. If pointer = 0 use blocking wait() semantics.
     * This is useful if we choose time interval and sometime need
     * infinity waiting.
     * @param time_is_relative = true time parameter should be time interval.
     * Implementation add this time interval to current system time.
     * @return bool: false if timeout.
     */
    bool
    timed_wait(
      const Gears::Time* time,
      bool time_is_relative = false)
      throw (Exception, Gears::Exception);
  };
}

//
// INLINES
//

namespace Gears
{
  //
  // class Conditional
  //

  inline
  constexpr
  Conditional::Conditional() throw ()
    : cond_ PTHREAD_COND_INITIALIZER
  {}
}

#endif /*GEARS_SYNC_CONDITION_HPP*/
