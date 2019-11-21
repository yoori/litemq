#ifndef SYNC_KEY_HPP
#define SYNC_KEY_HPP

#include <pthread.h>

#include "Errno.hpp"

#include "Uncopyable.hpp"

namespace Gears
{
  /**
   * Performs access to thread-specific data stored as pointers
   */
  template <typename Data>
  class ThreadKey : private Gears::Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);

    /**
     * Create thread-specific key
     * @param destructor optional destructor called for data on thread
     * termination
     */
    explicit
    ThreadKey(void (*destructor)(void*) = 0) throw (Exception);

    /**
     * Store data for the current thread
     * @param data to store
     */
    void
    set_data(Data* data) throw (Exception);

    /**
     * Get stored data for the current thread
     * @return stored data
     */
    Data*
    get_data() throw ();

  private:
    pthread_key_t key_;
  };
}

namespace Gears
{
  template <typename Data>
  ThreadKey<Data>::ThreadKey(void (*destructor)(void*)) throw (Exception)
  {
    static const char* FUN = "ThreadKey::ThreadKey()";

    const int RES = pthread_key_create(&key_, destructor);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        ": Failed to create key");
    }
  }

  template <typename Data>
  void
  ThreadKey<Data>::set_data(Data* data) throw (Exception)
  {
    static const char* FUN = "ThreadKey::set_data()";

    const int RES = pthread_setspecific(key_, data);
    if (RES)
    {
      Gears::throw_errno_exception<Exception>(RES, FUN,
        ": Failed to set data");
    }
  }

  template <typename Data>
  Data*
  ThreadKey<Data>::get_data() throw ()
  {
    return static_cast<Data*>(pthread_getspecific(key_));
  }
}

#endif
