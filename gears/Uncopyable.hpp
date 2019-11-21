#ifndef GEARS_UNCOPYABLE_HPP_
#define GEARS_UNCOPYABLE_HPP_

#include <utility>

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
#ifndef constexpr
#define constexpr
#endif
#endif

namespace Gears
{
  /**
   * Private inheritance of this class makes impossible the usage of
   * the implicit copy constructor and the assignment operator of the
   * derived class
   */
  class Uncopyable
  {
  protected:
    constexpr
    Uncopyable() = default;

    ~Uncopyable() = default;

    Uncopyable(Uncopyable&) = delete;

    Uncopyable(const Uncopyable&) = delete;

    Uncopyable(Uncopyable&&) = delete;

    Uncopyable(const Uncopyable&&) = delete;

    void
    operator=(Uncopyable&) = delete;

    void
    operator=(const Uncopyable&) = delete;

    void
    operator=(Uncopyable&&) = delete;

    void
    operator=(const Uncopyable&&) = delete;
  };
}

#endif
