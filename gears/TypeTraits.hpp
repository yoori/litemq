#ifndef GEARS_TYPE_TRAITS_HPP
#define GEARS_TYPE_TRAITS_HPP

#include <limits>

namespace Gears
{
  template<typename Type>
  struct RemoveConst
  {
    typedef Type Result;
  };

  template<typename Type>
  struct RemoveConst<const Type>
  {
    typedef Type Result;
  };

  template<typename Integer>
  Integer
  safe_next(Integer number) throw ()
  {
    return number < std::numeric_limits<Integer>::max() ? number + 1 : number;
  }
}

#endif /*GEARS_TYPE_TRAITS_HPP*/
