#ifndef GEARS_ATOMICCOUNTER_HPP
#define GEARS_ATOMICCOUNTER_HPP

#if __APPLE__
#  include <libkern/OSAtomic.h>
#else
// GCC
#  include <signal.h>

#  ifdef __GNUC__
#    if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2
#      include <ext/atomicity.h>
#    else
#      include <bits/atomicity.h>
#    endif
#  endif
#endif

#include "Uncopyable.hpp"

namespace Gears
{
  typedef volatile sig_atomic_t SigAtomicType;

  class AtomicCounter: public Uncopyable
  {
  public:
    AtomicCounter() {}

    explicit AtomicCounter(int value);

    void add(int value);

    int fetch_and_add(int value);

    int add_and_fetch(int value);

    AtomicCounter& operator=(int value);

    AtomicCounter& operator++();

    AtomicCounter& operator--();

    long operator++(int);

    long operator--(int);

    AtomicCounter& operator+=(int value);

    AtomicCounter& operator-=(int value);

    operator int() const;

  protected:
#if __APPLE__
    typedef int32_t Word;
#else
    // GCC
    typedef _Atomic_word Word;
#endif

  private:
    AtomicCounter(const AtomicCounter&);
    AtomicCounter& operator=(const AtomicCounter&);

  private:
    mutable volatile Word value_;
  };
}

namespace Gears
{
  inline
  AtomicCounter::AtomicCounter(int value)
    : value_(value)
  {}

#if __APPLE__
  inline void
  AtomicCounter::add(int value)
  {
    ::OSAtomicAdd32(value, &value_);
  }

  inline int
  AtomicCounter::fetch_and_add(int value)
  {
    return ::OSAtomicAdd32(value, &value_) - value;
  }

  inline int
  AtomicCounter::add_and_fetch(int value)
  {
    return ::OSAtomicAdd32(value, &value_);
  }

#else
  // GCC
  inline void
  AtomicCounter::add(int value)
  {
    __gnu_cxx::__atomic_add(&value_, value);
  }
  
  inline int
  AtomicCounter::fetch_and_add(int value)
  {
    return __gnu_cxx::__exchange_and_add(&value_, value);
  }
  
  inline int
  AtomicCounter::add_and_fetch(int value)
  {
    return __gnu_cxx::__exchange_and_add(&value_, value) + value;
  }

#endif

  inline AtomicCounter&
  AtomicCounter::operator=(int value)
  {
    value_ = value;
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator++()
  {
    add(1);
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator--()
  {
    add(-1);
    return *this;
  }

  inline long
  AtomicCounter::operator++(int)
  {
    return fetch_and_add(1);
  }

  inline long
  AtomicCounter::operator--(int)
  {
    return fetch_and_add(-1);
  }

  inline AtomicCounter&
  AtomicCounter::operator+=(int value)
  {
    add(value);
    return *this;
  }

  inline AtomicCounter&
  AtomicCounter::operator-=(int value)
  {
    add(-value);
    return *this;
  }

  inline
  AtomicCounter::operator int() const
  {
    return const_cast<AtomicCounter*>(this)->fetch_and_add(0);
  }
}

#endif /*GEARS_ATOMICCOUNTER_HPP*/
