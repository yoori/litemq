#ifndef GEARS_ASCII_HPP_
#define GEARS_ASCII_HPP_

#include <string>
#include <functional>
#include <algorithm>
#include <cctype>

#include <inttypes.h>
#include <limits.h>

#include "Exception.hpp"
#include "SubString.hpp"

namespace Gears
{
  /**
   * Contain routines that manipulate only with ASCII encoded data.
   * Bytes range in 0-127.
   */
  namespace Ascii
  {
    /**
     * these functions don't use stdlib codepage and
     * affect only for (0-127) chars.
     */
    char
    to_lower(char ch) throw ();

    char
    to_upper(char ch) throw ();

    void
    to_lower(std::string& dest) throw ();

    void
    to_upper(std::string& dest) throw ();

    template <typename Iterator>
    void
    to_lower(Iterator first, Iterator last) throw (Gears::Exception);

    template <typename Iterator>
    void
    to_upper(Iterator first, Iterator last) throw (Gears::Exception);

    namespace Category
    {
      /**
       * Category inherits a predicate and allows to search strings
       * for belongins/nonbelongins chars
       */
      template <typename Predicate>
      struct Category :
        public std::unary_function<char, bool>,
        public Predicate
      {
      public:
        /**
         * Constructor
         * Calls default constructor of Predicate
         */
        Category() throw (Gears::Exception);

        /**
         * Constructor
         * Calls constructor of Predicate with data
         * @param data data for constructor of Predicate
         */
        template <typename T>
        Category(const T& data) throw (Gears::Exception);

        /**
         * Constructor
         * Calls constructor of Predicate with data1 and data2
         * @param data1 data for constructor of Predicate
         * @param data2 data for constructor of Predicate
         */
        template <typename T1, typename T2>
        Category(const T1& data1, const T2& data2) throw (Gears::Exception);

        /**
         * Constructor
         * Calls constructor of Predicate with data1, data2 and data3
         * @param data1 data for constructor of Predicate
         * @param data2 data for constructor of Predicate
         * @param data3 data for constructor of Predicate
         */
        template <typename T1, typename T2, typename T3>
        Category(const T1& data1, const T2& data2, const T3& data3)
          throw (Gears::Exception);

        /**
         * Checks if character is in the set
         * @param ch character to test
         * @return Presence of the character in the set
         */
        bool
        is_owned(char ch) const throw ();

        /**
         * Checks category for emptiness
         * @return true if category has no symbol inside
         */
        bool
        empty() const throw ();

        /**
         * Finds the first character in the string which belongs to the set
         * @param str the string to search in
         * @return Pointer to found character or NULL if none
         */
        const char*
        find_owned(const char* str) const throw ();

        /**
         * Finds the first character in the string which belongs to the set
         * @param begin beginning of the string to search in
         * @param end end of the string to search in
         * @param octets_length used for storing length of symbol in octets
         * @return Pointer to found character or end if none
         */
        const char*
        find_owned(const char* begin, const char* end,
          unsigned long* octets_length = 0) const throw ();

        /**
         * Finds the first character in the string which doesn't belong
         * to the set
         * @param str the string to search in
         * @return Pointer to found character or NULL if none
         */
        const char*
        find_nonowned(const char* str) const throw ();

        /**
         * Finds the first character in the string which does not belong
         * to the set
         * @param begin beginning of the string to search in
         * @param end end of the string to search in
         * @return Pointer to found character or end if none
         */
        const char*
        find_nonowned(const char* begin, const char* end) const throw ();

        /**
         * Finds the last character in the string which belongs to the set
         * @param pos The pointer to char beyond the string to search in
         * @param start The pointer to begin of the string to search in.
         * Interval [start, pos) will be looked in.
         * @return Pointer to found character or original value of pos if
         * none.
         */
        const char*
        rfind_owned(const char* pos, const char* start) const throw ();

        /**
         * Finds the last character in the string which does not belong
         * to the set
         * @param pos The pointer to char beyond  the string to search in
         * @param start The pointer to begin of the string to search in.
         * Interval [start, pos) will be looked in.
         * @return Pointer to found character or original value of pos if
         * none.
         */
        const char*
        rfind_nonowned(const char* pos, const char* start) const throw ();
      };

      /**
       * Predicate for Category
       * Contains a table of symbols inside
       */
      class CharTable
      {
      public:
        /**
         * Default constructor.
         * Does not initialize the object.
         */
        CharTable()
          throw ();

        /**
         * Constructor
         * @param str List of characters in the set. '-' may be used for
         * ranges. To specify dash character it should be the first or the
         * last in the passed string or within a range
         * @param check_zero If nul character should be included in the set
         */
        explicit
        CharTable(const char* str, bool check_zero = false)
          throw ();

        /**
         * Constructor
         * Created object is a union of passed ones
         * @param first first object to unite
         * @param second second object to unite
         */
        CharTable(const CharTable& first, const CharTable& second)
          throw ();

        /**
         * Constructor
         * Created object is a union of passed ones
         * @param first first object to unite
         * @param second second object to unite
         * @param third third object to unite
         */
        CharTable(const CharTable& first, const CharTable& second,
          const CharTable& third)
          throw ();

        /**
         * Constructor
         * Created object is a predicate result
         * @param predicate predicate for initialization
         */
        template <typename Predicate>
        explicit
        CharTable(Predicate predicate) throw ();

        bool
        operator ()(char ch) const throw ();

      private:
        bool table_[256];
      };

      /**
       * Predicate for Category
       * Checks the one symbol
       */
      template <const char SYMBOL>
      struct Char1
      {
        /**
         * Checks if ch equals to SYMBOL
         * @return if ch equals to SYMBOL
         */
        bool
        operator ()(char ch) const throw ();
      };

      /**
       * Predicate for Category
       * Checks the two symbols
       */
      template <const char SYMBOL1, const char SYMBOL2>
      struct Char2
      {
        /**
         * Checks if ch equals to SYMBOL1 or SYMBOL2
         * @return if ch equals to SYMBOL1 or SYMBOL2
         */
        bool
        operator ()(char ch) const throw ();
      };

      /**
       * Predicate for Category
       * Checks the three symbols
       */
      template <const char SYMBOL1, const char SYMBOL2, const char SYMBOL3>
      struct Char3
      {
        /**
         * Checks if ch equals to SYMBOL1, SYMBOL2 or SYMBOL3
         * @return if ch equals to SYMBOL1, SYMBOL2 or SYMBOL3
         */
        bool
        operator ()(char ch) const throw ();
      };
    }

    // Quick access classes for different Categories
    typedef Category::Category<Category::CharTable> CharCategory;
    template <const char SYMBOL>
    struct Char1Category :
      public Category::Category<Category::Char1<SYMBOL> >
    {};

    template <const char SYMBOL1, const char SYMBOL2>
    struct Char2Category :
      public Category::Category<Category::Char2<SYMBOL1, SYMBOL2> >
    {};

    template <const char SYMBOL1, const char SYMBOL2, const char SYMBOL3>
    struct Char3Category :
      public Category::Category<Category::Char3<SYMBOL1, SYMBOL2, SYMBOL3> >
    {};

    /// Small and capital Latin letters.
    extern const CharCategory ALPHA;
    /// Arabic numerals
    extern const CharCategory NUMBER;
    /// Arabic numerals and Latin letters
    extern const CharCategory ALPHA_NUM;
    /// Numerals used in octal notation
    extern const CharCategory OCTAL_NUMBER;
    /// Numerals and letters used in hexadecimal notation
    extern const CharCategory HEX_NUMBER;
    /// Space characters
    extern const CharCategory SPACE;
    /// Symbols are used in regular expressions.
    extern const CharCategory REGEX_META;

    /**
     * Finds and replaces all sequences of chars from CharCategory
     * to replace string.
     * @param dest result put here.
     * @param str source string pointer
     * @param size length of processing string.
     * @param replacement - string that replace sequences of.
     * @param to_replace all sequences of chars from this category, will
     * be replaced by replace.
     */
    void
    flatten(std::string& dest,
      const char* str,
      size_t size,
      const SubString& replacement = SubString(" ", 1),
      const CharCategory& to_replace = SPACE)
      throw (Gears::Exception);

    /**
     * Finds and replaces all sequences of chars from CharCategory
     * to replace string.
     * @param dest result put here.
     * @param first source string pointer
     * @param last source string pointer that signal stop processing.
     * @param replacement string that replace sequences of.
     * @param to_replace all sequences of chars from this category, will
     * be replaced by replace.
     */
    void
    flatten(std::string& dest,
      const char* first,
      const char* last,
      const SubString& replacement = SubString(" ", 1),
      const CharCategory& to_replace = SPACE)
      throw (Gears::Exception);

    /**
     * Compare caseless two non zero terminated string
     * (strings can contain 0 inside).
     * Check str1 ? str2 where ? may be '<', '=' and '>'
     * @param str1 pointer to first string
     * @param len1 first string length
     * @param str2 pointer to second string
     * @param len2 second string length
     * @return return an integer less than, equal to,
     * or greater than zero, if str1 is found, respectively,
     * to be less than, to match, or be greater than str2
     */
    int
    compare_caseless(const char* str1,
      std::size_t len1,
      const char* str2,
      std::size_t len2)
      throw ();

    /**
     * Checks two non zero terminated strings on equality
     * (strings can contain 0 inside).
     * Check str1 == str2
     * @param str1 pointer to first string
     * @param len1 first string length
     * @param str2 pointer to second string
     * @param len2 second string length
     * @return true if lengths of string are equal and str1 == str2
     * ignoring case, false if not.
     */
    bool
    equal_caseless(const char* str1,
      std::size_t len1,
      const char* str2,
      std::size_t len2)
      throw ();

    /**
     * Checks SubString and non zero terminated string on equality
     * (strings can contain 0 inside).
     * Check str == str2
     * @param str SubString to be first argue in comparison.
     * @param str2 pointer to second string
     * @param len2 second string length
     * @return true if lengths of string are equal and str == str2
     * ignoring case, false if not.
     */
    bool
    equal_caseless(const SubString& str,
      const char* str2,
      std::size_t len2)
      throw ();

    /**
     * Checks SubStrings on equality ignore possible \\0 characters
     * and ignore letters case.
     * (strings can contain 0 inside).
     * Check str == str2
     * @param str1 SubString to be first argue in comparison.
     * @param str2 SubString to be second argue in comparison.
     * @return true if lengths of string are equal and str1 == str2
     * ignoring case, false if not.
     */
    bool
    equal_caseless(const SubString& str1, const SubString& str2)
      throw ();

    /// Small and capital Latin letters.
    extern const CharCategory ALPHA;
    /// Arabic numerals
    extern const CharCategory NUMBER;
    /// Arabic numerals and Latin letters
    extern const CharCategory ALPHA_NUM;
    /// Numerals used in octal notation
    extern const CharCategory OCTAL_NUMBER;
    /// Numerals and letters used in hexadecimal notation
    extern const CharCategory HEX_NUMBER;
    /// Space characters
    extern const CharCategory SPACE;
    /// Symbols are used in regular expressions.
    extern const CharCategory REGEX_META;

    typedef const Char1Category<':'> SepColon;
    typedef const Char1Category<','> SepComma;
    typedef const Char1Category<'.'> SepPeriod;
    typedef const Char1Category<'-'> SepMinus;
    typedef const Char1Category<';'> SepSemCol;
    typedef const Char1Category<'&'> SepAmp;
    typedef const Char1Category<' '> SepSpace;
    typedef const Char1Category<'='> SepEq;
    typedef const Char1Category<'/'> SepSlash;
    typedef const Char1Category<'#'> SepHash;
    typedef const Char1Category<'|'> SepBar;
    typedef const Char1Category<'\n'> SepNL;
    typedef const Char1Category<'\t'> SepTab;
    typedef const Char1Category<'_'> SepUnderscore;

    /**
     * Helper object to compare ASCII strings caselessly.
     */
    struct Caseless
    {
    public:
      /**
       * Constructor.
       * Calls strlen().
       * @param str string to store for comparison
       */
      explicit
      Caseless(const char* str) throw ();
      /**
       * Constructor
       * @param str string to store for comparison
       */
      explicit
      Caseless(const SubString& str) throw ();

      /**
       * Compares the stored string with the given one.
       * @param str string to compare with
       * @return return an integer less than, equal to,
       * or greater than zero, if the stored string is found,
       * respectively, to be less than, to match, or be
       * greater than str
       */
      int
      compare(const SubString& str) const throw ();

      /**
       * Checks SubStrings on equality ignoring letters case.
       * @param str string to compare with
       * @return true if lengths of string are equal and
       * the content of stored string equals to str
       * ignoring case, false if not.
       */
      bool
      equal(const SubString& str) const throw ();

      /**
       * Checks if str begins from the stored string.
       * @param str string to check
       * @return true if str size is not less than the stored string
       * size and those first letters are equal ignoring case.
       */
      bool
      start(const SubString& str) const throw ();

      SubString str;
    };
  }
}

namespace Gears
{
  namespace Ascii
  {
    /**
     * Contain names of tables, need for effective data manipulation
     */
    namespace Tables
    {
      /// Table convert ASCII (0-127) data to lower latin letters
      extern const char ASCII_TOLOWER_TABLE[256];
      /// Table convert ASCII (0-127) data to upper latin letters
      extern const char ASCII_TOUPPER_TABLE[256];
    }

    inline void
    flatten(std::string& dest, const char* first, const char* last,
      const SubString& replacement, const CharCategory& to_replace)
      throw (Gears::Exception)
    {
      const char* const REPLACEMENT_DATA = replacement.data();
      const size_t REPLACEMENT_SIZE = replacement.size();
      const char* const REPLACEMENT_END = REPLACEMENT_DATA + REPLACEMENT_SIZE;
      dest.resize((last - first) *
        (REPLACEMENT_SIZE ? REPLACEMENT_SIZE : 1));
      char* out = &dest[0];
      const char* current;

      while (first != last)
      {
        current = to_replace.find_owned(first, last);
        // last if haven't spaces
        // copy text before space
        out = std::copy(first, current, out);

        if (current == last)
        {
          break;
        }
        out = std::copy(REPLACEMENT_DATA, REPLACEMENT_END, out);
        first = to_replace.find_nonowned(current, last);
      }
      dest.resize(out - &dest[0]);
    }

    inline void
    flatten(std::string& dest, const char* str, size_t size,
      const SubString& replacement, const CharCategory& to_replace)
      throw (Gears::Exception)
    {
      flatten(dest, str, str + size, replacement, to_replace);
    }

    inline char
    to_lower(char ch) throw ()
    {
      return Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(ch)];
    }

    inline char
    to_upper(char ch) throw ()
    {
      return Tables::ASCII_TOUPPER_TABLE[static_cast<uint8_t>(ch)];
    }

    template <typename Iterator>
    void
    to_lower(Iterator first, Iterator last) throw (Gears::Exception)
    {
      for (; first != last; ++first)
      {
        *first =
          Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*first)];
      }
    }

    template <typename Iterator>
    void
    to_upper(Iterator first, Iterator last) throw (Gears::Exception)
    {
      for (; first != last; ++first)
      {
        *first =
          Tables::ASCII_TOUPPER_TABLE[static_cast<uint8_t>(*first)];
      }
    }

    inline void
    to_lower(std::string& dest) throw ()
    {
      to_lower(dest.begin(), dest.end());
    }

    inline void
    to_upper(std::string& dest) throw ()
    {
      to_upper(dest.begin(), dest.end());
    }

    inline int
    compare_caseless(const char* str1, std::size_t len1,
      const char* str2, std::size_t len2)
      throw ()
    {
      std::size_t len = std::min(len1, len2);
      if (str1 == str2 || len == 0)
      {
        if (len1 != len2)
        {
          return len1 < len2 ? -1 : 1;
        }
        return 0;
      }

      int result;
      while (!(result =
        Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str1++)] -
        Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str2++)]))
      {
        if (!--len)
        {
          break;
        }
      }

      if (result == 0 && len1 != len2)
      {
        return len1 < len2 ? -1 : 1;
      }
      return result;
    }

    inline bool
    equal_caseless(const char* str1, std::size_t len1,
      const char* str2)
      throw ()
    {
      for (register const char* const END = str1 + len1; str1 != END;)
      {
        register const char CH(*str2++);
        if (!CH || Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(CH)] !=
          Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str1++)])
        {
          return false;
        }
      }

      return !*str2;
    }

    inline bool
    equal_caseless(const char* str1, std::size_t len1,
      const char* str2, std::size_t len2)
      throw ()
    {
      if (len1 != len2)
      {
        return false;
      }
      if (str1 == str2 || len1 == 0)
      {
        return true;
      }

      while (Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str1++)] ==
        Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str2++)])
      {
        if (!--len1)
        {
          return true;
        }
      }

      return false;
    }

    inline bool
    equal_caseless(const SubString& str,
      const char* str2, std::size_t len2)
      throw ()
    {
      return equal_caseless(str.data(), str.size(), str2, len2);
    }

    inline bool
    equal_caseless(const SubString& str, const char* str2)
      throw ()
    {
      return equal_caseless(str.data(), str.size(), str2);
    }

    inline bool
    equal_caseless(const SubString& str1,
      const SubString& str2)
      throw ()
    {
      return equal_caseless(str1.data(), str1.size(),
        str2.data(), str2.size());
    }

    namespace Category
    {
      //
      // Category class
      //
      template <typename Predicate>
      Category<Predicate>::Category() throw (Gears::Exception)
      {}

      template <typename Predicate>
      template <typename T>
      Category<Predicate>::Category(const T& data) throw (Gears::Exception)
        : Predicate(data)
      {}

      template <typename Predicate>
      template <typename T1, typename T2>
      Category<Predicate>::Category(const T1& data1, const T2& data2)
        throw (Gears::Exception)
        : Predicate(data1, data2)
      {}

      template <typename Predicate>
      template <typename T1, typename T2, typename T3>
      Category<Predicate>::Category(const T1& data1, const T2& data2,
        const T3& data3) throw (Gears::Exception)
        : Predicate(data1, data2, data3)
      {}

      template <typename Predicate>
      bool
      Category<Predicate>::is_owned(char ch) const throw ()
      {
        return Predicate::operator ()(ch);
      }

      template <typename Predicate>
      bool
      Category<Predicate>::empty() const throw ()
      {
        for (register char ch = CHAR_MIN; ; ch++)
        {
          if (is_owned(ch))
          {
            return false;
          }
          if (ch == CHAR_MAX)
          {
            break;
          }
        }
        return true;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::find_owned(const char* str) const throw ()
      {
        for (register char ch; (ch = *str) != '\0'; str++)
        {
          if (is_owned(ch))
          {
            return str;
          }
        }
        return is_owned('\0') ? str : 0;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::find_owned(const char* str, const char* end,
        unsigned long* octets_length) const throw ()
      {
        for (; str != end; ++str)
        {
          if (is_owned(*str))
          {
            if (octets_length)
            {
              *octets_length = 1;
            }

            return str;
          }
        }
        return end;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::find_nonowned(const char* str) const
        throw ()
      {
        for (register char ch; (ch = *str) != '\0'; str++)
        {
          if (!is_owned(ch))
          {
            return str;
          }
        }
        return !is_owned('\0') ? str : 0;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::find_nonowned(const char* str,
        const char* end) const
        throw ()
      {
        for (; str != end; ++str)
        {
          if (!is_owned(*str))
          {
            return str;
          }
        }
        return end;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::rfind_owned(const char* pos,
        const char* start) const
        throw ()
      {
        const char* const NOT_FOUND = pos;
        while (start != pos)
        {
          if (is_owned(*--pos))
          {
            return pos;
          }
        }
        return NOT_FOUND;
      }

      template <typename Predicate>
      const char*
      Category<Predicate>::rfind_nonowned(const char* pos,
        const char* start) const
        throw ()
      {
        const char* const NOT_FOUND = pos;
        while (start != pos)
        {
          if (!is_owned(*--pos))
          {
            return pos;
          }
        }
        return NOT_FOUND;
      }

      //
      // CharTable class
      //
      inline
      CharTable::CharTable() throw ()
      {
      }

      template <typename Predicate>
      CharTable::CharTable(Predicate predicate) throw ()
      {
        for (register int i = 0; i < 256; i++)
        {
          table_[i] = predicate(i);
        }
      }

      inline bool
      CharTable::operator ()(char ch) const throw ()
      {
        return table_[static_cast<uint8_t>(ch)];
      }

      //
      // Char1 class
      //
      template <const char SYMBOL>
      bool
      Char1<SYMBOL>::operator ()(char ch) const throw ()
      {
        return ch == SYMBOL;
      }

      //
      // Char2 class
      //
      template <const char SYMBOL1, const char SYMBOL2>
      bool
      Char2<SYMBOL1, SYMBOL2>::operator ()(char ch) const throw ()
      {
        return ch == SYMBOL1 || ch == SYMBOL2;
      }

      //
      // Char3 class
      //
      template <const char SYMBOL1, const char SYMBOL2, const char SYMBOL3>
      bool
      Char3<SYMBOL1, SYMBOL2, SYMBOL3>::operator ()(char ch) const throw ()
      {
        return ch == SYMBOL1 || ch == SYMBOL2 || ch == SYMBOL3;
      }
    }

    // Caseless impl
    inline
    Caseless::Caseless(const char* str) throw ()
      : str(str)
    {}

    inline
    Caseless::Caseless(const SubString& str) throw ()
      : str(str)
    {}

    inline
    int
    Caseless::compare(const SubString& str) const throw ()
    {
      const char* str1 = this->str.data();
      const char* str2 = str.data();
      size_t len1 = this->str.size();
      size_t len2 = str.size();
      size_t len = std::min(len1, len2);
      if (str1 != str2 && len)
      {
        while (len--)
        {
          int result = static_cast<int>(Tables::ASCII_TOLOWER_TABLE[
            static_cast<uint8_t>(*str1++)]) -
            Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str2++)];
          if (result)
          {
            return result;
          }
        }
      }

      return len1 != len2 ? len1 < len2 ? -1 : 1 : 0;
    }

    inline
    bool
    Caseless::equal(const SubString& str) const throw ()
    {
      const char* str1 = str.data();
      const char* str2 = this->str.data();
      size_t len = str.size();
      if (len != this->str.size())
      {
        return false;
      }
      if (str1 == str2 || !len)
      {
        return true;
      }

      while (Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str1++)] ==
        Tables::ASCII_TOLOWER_TABLE[static_cast<uint8_t>(*str2++)])
      {
        if (!--len)
        {
          return true;
        }
      }

      return false;
    }

    inline
    bool
    Caseless::start(const SubString& str) const throw ()
    {
      return equal(str.substr(0, this->str.size()));
    }

    inline
    bool
    operator ==(const SubString& str, const Caseless& cl)
      throw ()
    {
      return cl.equal(str);
    }

    inline
    bool
    operator ==(const Caseless& cl, const SubString& str)
      throw ()
    {
      return str == cl;
    }

    inline
    bool
    operator !=(const SubString& str, const Caseless& cl)
      throw ()
    {
      return !(str == cl);
    }

    inline
    bool
    operator !=(const Caseless& cl, const SubString& str)
      throw ()
    {
      return str != cl;
    }
  }
}

#endif /*GEARS_ASCII_HPP_*/
