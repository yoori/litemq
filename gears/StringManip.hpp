#ifndef GEARS_STRINGMANIP_HPP_
#define GEARS_STRINGMANIP_HPP_

#include <assert.h>
#include <vector>

#include "Exception.hpp"
#include "SubString.hpp"
#include "AsciiStringManip.hpp"

namespace Gears
{
  /**
   * Contain general string manipulation routines
   */

  typedef std::vector<unsigned char> ArrayByte;

  namespace StringManip
  {
    DECLARE_EXCEPTION(InvalidFormatException, Gears::DescriptiveException);

    uint32_t
    crc32(uint32_t crc, const void* data, size_t size) throw ();

    /**
     * Encodes data into hex string
     * @param data source data
     * @param size data size
     * @param skip_leading_zeroes if skip all leading zeroes
     * @return encoded hex string
     */
    std::string
    hex_encode(
      const unsigned char* data,
      size_t size,
      bool skip_leading_zeroes) throw (Gears::Exception);

    /**
     * Encodes data into hex string in low case
     * @param data source data
     * @param size data size
     * @param skip_leading_zeroes if skip all leading zeroes
     * @return encoded hex string
     */
    std::string
    hex_low_encode(
      const unsigned char* data,
      size_t size,
      bool skip_leading_zeroes) throw (Gears::Exception);

    /**
     * Decodes hex src into array of bytes
     * @param src source string
     * @param dst destination array of bytes
     * @param allow_odd_string if odd length string is allowed
     * @return length of destination array
     */
    size_t
    hex_decode(
      ArrayByte& dst,
      SubString src,
      bool allow_odd_string = false)
      throw (Gears::Exception, InvalidFormatException);

    /**
     * Removes characters defined in a set of symbols
     * from the beginning and end of the string
     * @param str trimming substring
     * @param trim_set Defines a set of characters to remove,
     * default value is AsciiStringManip::SPACE
     */
    void
    trim(SubString& str,
      const Ascii::CharCategory& trim_set =
        Ascii::SPACE)
      throw ();

    template<typename Integer>
    bool
    str_to_int(const SubString& str, Integer& value) throw ();

    /**
     * Converts integer value into string
     * @param value value to convert
     * @param str buffer to convert to (zero terminated)
     * @param size its size
     * @return number of characters written (without trailing zero)
     * (0 indicates error)
     */
    template <typename Integer>
    size_t
    int_to_str(Integer value, char* str, size_t size) throw ();

    void
    concat(char* buffer, size_t size)
      throw ();

    /**
     * Safely concatenates several strings to the string buffer.
     * @param buffer string buffer to concatenate to
     * @param size the size of the buffer
     * @param f string to append
     * @param args strings to append
     */
    template <typename First, typename... Args>
    void
    concat(char* buffer, size_t size, First f, Args&&... args)
      throw ();
  } // namespace StringManip
} // namespace Gears

// implementations
namespace Gears
{
namespace StringManip
{
  namespace IntToStrHelper
  {
    template <typename Integer, const bool is_signed>
    struct IntToStrSign;

    template <typename Integer>
    struct IntToStrSign<Integer, false>
    {
      static
      size_t
      convert(Integer value, char* str) throw ();
    };

    template <typename Integer>
    size_t
    IntToStrSign<Integer, false>::convert(Integer value, char* str)
      throw ()
    {
      char* ptr = str;
      do
      {
        *ptr++ = '0' + value % 10;
      }
      while (value /= 10);
      size_t size = ptr - str;
      for (*ptr-- = '\0'; str < ptr; str++, ptr--)
      {
        std::swap(*str, *ptr);
      }
      return size;
    }

    template <typename Integer>
    struct IntToStrSign<Integer, true>
    {
      static
      size_t
      convert(Integer value, char* str) throw ();
    };

    template <typename Integer>
    size_t
    IntToStrSign<Integer, true>::convert(Integer value, char* str)
      throw ()
    {
      if (value < -std::numeric_limits<Integer>::max())
      {
        return 0;
      }

      if (value < 0)
      {
        *str = '-';
        return IntToStrSign<Integer, false>::convert(-value, str + 1) + 1;
      }

      return IntToStrSign<Integer, false>::convert(value, str);
    }
  }

  template <typename Integer>
  bool
  str_to_int(const SubString& str, Integer& value) throw ()
  {
    const char* src = str.begin();
    const char* const END = str.end();
    if(src == END)
    {
      return false;
    }

    bool negative = false;
    switch (*src)
    {
    case '-':
      if(!std::numeric_limits<Integer>::is_signed)
      {
        return false;
      }
      negative = true;
    case '+':
      if(++src == END)
      {
        return false;
      }
    }

    value = 0;
    const Integer LIMIT = std::numeric_limits<Integer>::max() / 10;
    if(negative)
    {
      do
      {
        unsigned char ch = static_cast<unsigned char>(*src) -
          static_cast<unsigned char>('0');
        if(ch > 9 || value < -LIMIT || (
          value == -LIMIT && ch > static_cast<unsigned char>(
            -(std::numeric_limits<Integer>::min() + LIMIT * 10))))
        {
          return false;
        }

        value = value * static_cast<Integer>(10) -
          static_cast<Integer>(ch);
      }
      while(++src != END);
    }
    else
    {
      do
      {
        unsigned char ch = static_cast<unsigned char>(*src) -
          static_cast<unsigned char>('0');
        if(ch > 9 || value > LIMIT || (value == LIMIT &&
          ch > static_cast<unsigned char>(
            std::numeric_limits<Integer>::max() - LIMIT * 10)))
        {
          return false;
        }

        value = value * static_cast<Integer>(10) +
          static_cast<Integer>(ch);
      }
      while(++src != END);
    }

    return true;
  }

  template <typename Integer>
  size_t
  int_to_str(Integer value, char* str, size_t size) throw ()
  {
    static_assert(std::numeric_limits<Integer>::is_integer,
      "Integer is not an integer type");

    if(size < std::numeric_limits<Integer>::digits10 + 3)
    {
      return 0;
    }

    return IntToStrHelper::IntToStrSign<Integer,
      std::numeric_limits<Integer>::is_signed>::convert(value, str);
  }

  inline
  size_t
  strlcpy(char* dst, const char* src, size_t size) throw ()
  {
    const char* const saved_src = src;

    if (size)
    {
      while (--size)
      {
        if (!(*dst++ = *src++))
        {
          return src - saved_src - 1;
        }
      }
      *dst = '\0';
    }

    while (*src++);

    return src - saved_src - 1;
  }

  inline
  size_t
  strlcat(char* dst, const char* src, size_t size) throw ()
  {
    const char* const saved_src = src;

    size_t dst_len = size;
    while (size-- && *dst)
    {
      dst++;
    }
    size++;
    dst_len -= size;

    if (size)
    {
      while (--size)
      {
        if (!(*dst++ = *src++))
        {
          return src - saved_src + dst_len - 1;
        }
      }
      *dst = '\0';
    }
    while (*src++);

    return src - saved_src + dst_len - 1;
  }

  inline
  size_t
  append(char* buffer, size_t size, const char* str) throw ()
  {
    size_t length = strlcpy(buffer, str, size);
    return length < size ? length : size;
  }

  inline
  size_t
  append(char* buffer, size_t size, char* str) throw ()
  {
    return append(buffer, size, const_cast<const char*>(str));
  }

  inline
  size_t
  append(char* buffer, size_t size, const SubString& str) throw ()
  {
    //assert(size);
    if (str.size() >= size)
    {
      std::char_traits<char>::copy(buffer, str.data(), size - 1);
      buffer[size - 1] = '\0';
      return size;
    }

    std::char_traits<char>::copy(buffer, str.data(), str.size());
    return str.size();
  }

  inline
  size_t
  append(char* buffer, size_t size, const std::string& str) throw ()
  {
    return append(buffer, size, SubString(str));
  }

  template <typename Integer>
  size_t
  append(char* buffer, size_t size, Integer integer) throw ()
  {
    size_t res = int_to_str(integer, buffer, size);
    if (!res)
    {
      //assert(size);
      *buffer = '\0';
      return size;
    }
    return res - 1;
  }

  inline
  void
  concat(char* buffer, size_t size)
    throw ()
  {
    assert(size);
    *buffer = '\0';
  }

  template <typename First, typename... Args>
  void
  concat(char* buffer, size_t size, First f, Args&&... args)
    throw ()
  {
    size_t length = append(buffer, size, f);
    if (length < size)
    {
      concat(buffer + length, size - length, args...);
    }
  }
}
}

#endif /*GEARS_STRINGMANIP_HPP_*/
