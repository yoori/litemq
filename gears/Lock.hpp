#ifndef GEARS_LOCK_HPP
#define GEARS_LOCK_HPP

#include <pthread.h>

#include "Uncopyable.hpp"

/**
 * StaticInitializedMutex: exclusive lock policy, must be initialized statically
 * Mutex: exclusive lock policy
 * SpinLock: exclusive spin lock policy
 * RWLock: read/write lock policy
 * NullLock: single thread policy
 */

#define STATIC_MUTEX_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }

namespace Gears
{
  template<typename LockType>
  class BaseReadGuard
  {
  public:
    BaseReadGuard(LockType& lock) throw()
      : lock_(lock)
    {
      lock_.lock_read();
    }

    ~BaseReadGuard() throw()
    {
      lock_.unlock();
    }

  protected:
    LockType& lock_;
  };

  template<typename LockType>
  class BaseWriteGuard
  {
  public:
    BaseWriteGuard(LockType& lock) throw()
      : lock_(lock)
    {
      lock_.lock();
    }

    ~BaseWriteGuard() throw()
    {
      lock_.unlock();
    }

  protected:
    LockType& lock_;
  };

  class StaticInitializedMutex
  {
    // for static initialization all members is public and no c-tor, d-tor
  public:
    typedef BaseWriteGuard<StaticInitializedMutex> ReadGuard;
    typedef BaseWriteGuard<StaticInitializedMutex> WriteGuard;

    void lock_read() throw();

    void lock() throw();

    void unlock() throw();

    pthread_mutex_t mutex_;
  };

  class RecursiveMutex:
    protected StaticInitializedMutex,
    private Uncopyable    
  {
    // for static initialization all members is public and no c-tor, d-tor
  public:
    typedef BaseWriteGuard<RecursiveMutex> ReadGuard;
    typedef BaseWriteGuard<RecursiveMutex> WriteGuard;

  public:
    RecursiveMutex() throw();

    RecursiveMutex(int pshared) throw();

    ~RecursiveMutex() throw();

    pthread_mutex_t& mutex_i() throw();

    using StaticInitializedMutex::lock_read;

    using StaticInitializedMutex::lock;

    using StaticInitializedMutex::unlock;
  };

  class Mutex:
    protected StaticInitializedMutex,
    private Uncopyable
  {
  public:
    typedef BaseWriteGuard<Mutex> ReadGuard;
    typedef BaseWriteGuard<Mutex> WriteGuard;

  public:
    Mutex() throw();

    Mutex(int pshared) throw();

    ~Mutex() throw();

    pthread_mutex_t& mutex_i() throw();

    using StaticInitializedMutex::lock_read;

    using StaticInitializedMutex::lock;

    using StaticInitializedMutex::unlock;
  };

  class RWLock: private Uncopyable
  {
  public:
    typedef BaseReadGuard<RWLock> ReadGuard;
    typedef BaseWriteGuard<RWLock> WriteGuard;

  public:
    RWLock() throw();

    RWLock(int pshared) throw();

    ~RWLock() throw();

    void lock_read() throw();

    void lock() throw();

    void unlock() throw ();

  protected:
    pthread_rwlock_t mutex_;
  };

#ifdef PTHREAD_SPINLOCK_DEFINED
  class SpinLock: private Uncopyable
  {
  public:
    typedef BaseWriteGuard<SpinLock> ReadGuard;
    typedef BaseWriteGuard<SpinLock> WriteGuard;

  public:
    SpinLock(int pshared = PTHREAD_PROCESS_PRIVATE) throw();

    ~SpinLock() throw();

    void lock() throw();

    void unlock() throw();

  protected:
    pthread_spinlock_t mutex_;
  };

#else
  typedef Mutex SpinLock;
#endif

  struct NullLock: private Uncopyable
  {
  public:
    struct NullGuard
    {
      NullGuard(NullLock&) throw() {}
    };

    typedef NullGuard ReadGuard;
    typedef NullGuard WriteGuard;

  public:
    NullLock() throw() {};

    void lock() throw() {};

    void unlock() throw() {};
  };
}

namespace Gears
{
  /* StaticInitializedMutex */
  inline
  void
  StaticInitializedMutex::lock_read() throw()
  {
    ::pthread_mutex_lock(&mutex_);
  }

  inline
  void
  StaticInitializedMutex::lock() throw()
  {
    ::pthread_mutex_lock(&mutex_);
  }

  inline
  void
  StaticInitializedMutex::unlock() throw()
  {
    ::pthread_mutex_unlock(&mutex_);
  }

  /* RecursiveMutex */
  inline
  RecursiveMutex::RecursiveMutex() throw()
  {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &mutex_attributes);
  }

  inline
  RecursiveMutex::RecursiveMutex(int pshared) throw()
  {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_setpshared(&mutex_attributes, pshared);
    pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &mutex_attributes);
  }

  inline
  RecursiveMutex::~RecursiveMutex() throw()
  {
    pthread_mutex_destroy(&mutex_);
  }

  inline
  pthread_mutex_t&
  RecursiveMutex::mutex_i() throw()
  {
    return mutex_;
  }

  /* Mutex */
  inline
  Mutex::Mutex() throw()
  {
    pthread_mutex_init(&mutex_, 0);
  }

  inline
  Mutex::Mutex(int pshared) throw()
  {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_setpshared(&mutex_attributes, pshared);
    pthread_mutex_init(&mutex_, &mutex_attributes);
  }

  inline
  Mutex::~Mutex() throw()
  {
    pthread_mutex_destroy(&mutex_);
  }

  inline
  pthread_mutex_t&
  Mutex::mutex_i() throw()
  {
    return mutex_;
  }

  /* RWLock */
  inline
  RWLock::RWLock() throw()
  {
    ::pthread_rwlock_init(&mutex_, 0);
  }

  inline
  RWLock::~RWLock() throw()
  {
    ::pthread_rwlock_destroy(&mutex_);
  }

  inline
  void
  RWLock::lock() throw()
  {
    ::pthread_rwlock_wrlock(&mutex_);
  }

  inline
  void
  RWLock::lock_read() throw()
  {
    ::pthread_rwlock_rdlock(&mutex_);
  }

  inline
  void
  RWLock::unlock() throw()
  {
    ::pthread_rwlock_unlock(&mutex_);
  }

#ifdef PTHREAD_SPINLOCK_DEFINED
  /* SpinLock */
  inline
  SpinLock::SpinLock(int pshared) throw()
  {
    ::pthread_spin_init(&mutex_, pshared);
  }

  inline
  SpinLock::~SpinLock() throw()
  {
    ::pthread_spin_destroy(&mutex_);
  }

  inline
  void
  SpinLock::lock() throw()
  {
    ::pthread_spin_lock(&mutex_);
  }

  inline
  void
  SpinLock::unlock() throw()
  {
    ::pthread_spin_unlock(&mutex_);
  }
#endif /*PTHREAD_SPINLOCK_DEFINED*/
}

#endif
