#ifndef GEARS_SUBSTRING_HPP_
#define GEARS_SUBSTRING_HPP_

#include <string>
#include <ostream>
//#define BASIC_STRING_INTERFACE_IMITATION
#ifdef BASIC_STRING_INTERFACE_IMITATION
#include <iterator>
#endif

#include "Exception.hpp"
#include "TypeTraits.hpp"

namespace Gears
{
  template <typename CharType>
  struct CharTraits: public std::char_traits<CharType>
  {
    static int
    compare(const CharType* str1, const CharType* str2, size_t size)
      throw ();

    static CharType*
    copy(CharType* str1, const CharType* str2, size_t size) throw ();
  };

  template <typename CharType>
  struct CheckerNone
  {
    typedef Exception OutOfRange;
    typedef Exception LogicError;

    static void
    check_position(size_t length, size_t pos, const char* error_func)
      throw ();

    static void
    check_pointer(const CharType* ptr, const char* error_func) throw ();

    static void
    check_pointer(const CharType* begin, const CharType* end,
      const char* error_func) throw ();

    static void
    check_pointer(const CharType* ptr, size_t count,
      const char* error_func) throw ();
  };

  template <typename CharType>
  class CheckerRough
  {
  public:
    /**
     * Raise if requested index of element out of allowed range.
     */
    DECLARE_EXCEPTION(OutOfRange, DescriptiveException);
    /**
     * Raise accordingly basic_string C++ standard.
     */
    DECLARE_EXCEPTION(LogicError, DescriptiveException);

    /**
     * Check awareness for shift begin on pos elements
     * @param length available length of substring
     * @param pos requested shift
     * @param error_func used to create exception if incorrect
     * position requested.
     * @return pos if pos have correct value, else raise exception.
     */
    static void
    check_position(size_t length, size_t pos, const char* error_func)
      throw (OutOfRange);

    /**
     * Check pointer != 0
     * @param ptr pointer to be checked
     * @param error_func used to create exception if ptr = 0
     */
    static void
    check_pointer(const CharType* ptr, const char* error_func)
      throw (LogicError);

    /**
     * Check that pointers define correct range of data
     * @param begin pointer to the begin of data to be checked
     * @param end pointer to the end of data to be checked
     * @param error_func used to create exception if ptr = 0
     */
    static void
    check_pointer(const CharType* begin, const CharType* end,
      const char* error_func)
      throw (LogicError);

    /**
     * Check pointer != 0 or count = 0
     * @param ptr pointer to be checked
     * @param count string length to be checked
     * @param error_func used to create exception if ptr = 0
     */
    static void
    check_pointer(const CharType* ptr, size_t count,
      const char* error_func)
      throw (LogicError);

  private:
    /**
     * Simply throw LogicError
     * @param error_func used to create LogicError
     */
    static void
    throw_logic_error_(const char* error_func)
      throw (LogicError);
  };

  /**
   * This class is designed to be a reference to the substring of somewhere
   * allocated string. It holds the pointer to the beginning and the length
   * of that substring and that's why can't live longer than the referred
   * string and its content.
   * Because of this the typedefs below use const char and const wchar_t
   * types.
   */
  template <typename CharType,
    typename Traits = std::char_traits<typename RemoveConst<CharType>::Result>,
    typename Checker = CheckerRough<CharType> >
  class BasicSubString
  {
  public:
    typedef typename Checker::OutOfRange OutOfRange;
    typedef typename Checker::LogicError LogicError;

    // typedefs
    typedef std::size_t SizeType;
    typedef std::ptrdiff_t DifferenceType;
    typedef CharType* Pointer;
    typedef const CharType* ConstPointer;
    typedef CharType& Reference;
    typedef const CharType& ConstReference;
    typedef CharType ValueType;
    typedef typename RemoveConst<ValueType>::Result
      BasicStringValueType;
    typedef std::basic_string<BasicStringValueType> BasicString;
#ifdef BASIC_STRING_INTERFACE_IMITATION
    typedef std::reverse_iterator<ConstPointer> ConstReverseIterator;
    typedef std::reverse_iterator<Pointer> ReverseIterator;
#endif
    static const SizeType NPOS = BasicString::npos;

    // Constructors

    /**
     * Constructor from std::basic_string<...> (std::string, std::wstring
     * for example). Create SubString covering the whole string.
     * @param str input SubString for coverage
     */
    template <typename BasicStringTraits, typename Allocator>
    BasicSubString(
      const std::basic_string<BasicStringValueType, BasicStringTraits,
        Allocator>& str) throw (Exception);

    /**
     * Constructor
     * @param ptr beginning of the SubString
     * @param count size of the SubString
     */
    BasicSubString(Pointer ptr, SizeType count) throw (LogicError);

    /**
     * Constructor
     * @param begin pointer to the beginning of the string
     * @param end pointer to the element beyond the last of that string
     */
    BasicSubString(Pointer begin, Pointer end) throw (LogicError);

    /**
     * Constructor with zero-terminated c-string.
     * WARNING! This constructor is explicit because it searches the
     * terminating zero as the end of passed the string.
     * Make sure that you want this function to be called.
     * Also there is no other way to assign a string with unspecified
     * length to the object but using of this constructor.
     * @param ptr beginning pointer of the string
     */
    explicit BasicSubString(Pointer ptr) throw (LogicError);

    /**
     * Construct an empty SubString.
     */
    explicit BasicSubString() throw ();

    /**
     * Constructor to avoid error code BasicSubString(0, 12345),
     * it leads to the compile error
     */
    BasicSubString(int, SizeType) throw ();

    /**
     * Assigns new range character to be the contents of a stored
     * SubString.
     * @param ptr source char sequence to save start position.
     * @param count number of elements in SubString.
     * @return reference on self
     */
    BasicSubString&
    assign(Pointer ptr, SizeType count) throw (LogicError);

    /**
     * Assigns new range character to be the contents of a stored
     * SubString.
     * @param begin pointer to the beginning of the string
     * @param end pointer to the element beyond the last of that string
     * @return reference on self
     */
    BasicSubString&
    assign(Pointer begin, Pointer end) throw (LogicError);

    /**
     * Assigns new range character to be the contents of a stored
     * SubString. Get SubString from existing SubString
     * @param str SubString source SubString
     * @param pos index of first character to store into SubString
     * @param count number of elements to store in SubString.
     * @return reference on self
     */
    BasicSubString&
    assign(const BasicSubString& str, SizeType pos, SizeType count)
      throw (OutOfRange);

    /**
     * Assigns SubString from source SubString
     * @param str source SubString
     * @return reference on self
     */
    BasicSubString&
    assign(const BasicSubString& str) throw ();

    /**
     * Returns a const reference to the element at a specified location
     * in the SubString.
     * @param pos specified index in a SubString.
     * @return const reference to SubString character.
     */
    ConstReference
    at(SizeType pos) const throw (OutOfRange);

    /**
     * Returns a reference to the element at a specified location
     * in the SubString.
     * @param pos specified index in a SubString.
     * @return reference to SubString character.
     */
    Reference
    at(SizeType pos) throw (OutOfRange);

    /**
     * Get begin pointer of SubString, on empty SubString equal 0.
     * @return a const iterator addressing the first element in the string.
     */
    ConstPointer
    begin() const throw ();

    /**
     * Get begin pointer of SubString, on empty SubString equal 0.
     * @return an iterator addressing the first element in the string.
     */
    Pointer
    begin() throw ();

    /**
     * Make SubString to be in empty state.
     */
    void
    clear() throw ();

    /**
     * Determines the effective length rlen of the strings to compare as
     * the smallest of size() and str.size(). The function then compares
     * the two strings by calling
     * char_traits<CharType>::compare(data(), str.data(), rlen).
     *
     * Compares a string with a specified string to determine
     * if the two strings are equal or if one is lexicographically
     * less than the other.
     * @param str SubString to compare with this
     * @return A negative value if the operand string is less than
     * the parameter string;
     * zero if the two strings are equal;
     * or a positive value if the operand string is greater than the
     * parameter string.
     *   Condition              Return result
     *    size() < str.size()   < 0
     *    size() == str.size()  0
     *    size() > str.size()   > 0
     */
    int
    compare(const BasicSubString& str) const throw ();

    /**
     * Compare with SubString of Substring
     * @param pos1 position in str to begin comparison from
     * @param count1 number of elements to be compared
     * @param str source SubString
     * @return an integer less than, equal to, or greater than zero
     * if this SubString (or the first count1 elements thereof) is found,
     * respectively, to be less than, to match, or be greater than str.
     */
    int
    compare(SizeType pos1, SizeType count1, const BasicSubString& str) const
      throw (OutOfRange);

    /**
     * Compare SubString of SubString with SubString of Substring.
     * @param pos1 position in str to begin comparison
     * @param count1 number of elements of str to be compared
     * @param str source SubString
     * @param pos2 position in this SubString to begin comparison
     * @param count2 number of elements of this SubString to be compared
     * @return an integer less than, equal to, or greater than zero
     * if part of this SubString is found,
     * respectively, to be less than, to match, or be greater
     * than part of str.
     */
    int
    compare(SizeType pos1, SizeType count1, const BasicSubString& str,
      SizeType pos2, SizeType count2) const throw (OutOfRange);


    /**
     * Compare with zero-terminated C-string. Doesn't calculate string
     * length before comparison.
     * @param str source C-string.
     * @return an integer less than, equal to, or greater than zero
     * if this SubString is found, respectively, to be less than,
     * to match, or be greater than zero-terminated string ptr.
     */
    int
    compare(ConstPointer str) const throw (LogicError);

    /**
     * Compare SubString of SubString with zero-terminated C-string.
     * Doesn't calculate str length before comparison.
     * @param pos1 position in this SubString to begin comparison
     * @param count1 number of elements of this SubString to be compared
     * @param str source C-string.
     * @return an integer less than, equal to, or greater than zero
     * if this part of SubString is found, respectively, to be less than,
     * to match, or be greater than zero-terminated string ptr.
     */
    int
    compare(SizeType pos1, SizeType count1, ConstPointer str) const
      throw (LogicError);

    /**
     * Compare SubString of SubString with count2 elements of
     * zero-terminated C-string.
     * Doesn't calculate string length before comparison.
     * @param pos1 position in this SubString to begin comparison
     * @param count1 number of elements of this SubString to be compared
     * @param str source C-string.
     * @param count2 maximum characters of ptr to copy.
     * @return an integer less than, equal to, or greater than zero
     * if this part of SubString is found, respectively, to be less than,
     * to match, or be greater than string ptr with length count2.
     */
    int
    compare(SizeType pos1, SizeType count1, ConstPointer str,
      SizeType count2) const throw (LogicError);

    /**
     * Compare SubString on equal with C-string.
     * @param str zero-terminated string to be compared with this.
     * @return true if SubString equal str, else return false
     */
    bool
    equal(ConstPointer str) const throw (LogicError);

    /**
     * Compare BasicSubString on equal with other BasicSubString.
     * SubStrings can contain internals zero.
     * @param str BasicSubString to be compared with this.
     * @return true if this equal str, else return false
     */
    bool
    equal(const BasicSubString& str) const throw ();

    /**
     * Copies at most a specified number of characters from an indexed
     * position in a source string to a target character array.
     * @param ptr target array
     * @param count the number of characters to be copied, at most,
     * from the source string.
     * @param pos the beginning position in the source string from which
     * copies are to be made.
     * @return the number of characters actually copied.
     */
    SizeType
    copy(BasicStringValueType* ptr, SizeType count, SizeType pos = 0) const
      throw (OutOfRange, LogicError);

    /**
     * Get pointer to the content of a SubString as an array of characters.
     * @return pointer to begin of array.
     */
    ConstPointer
    data() const throw ();

    /**
     * Tests whether the substring contains characters or not.
     * @return substring emptiness status
     */
    bool
    empty() const throw ();

    /**
     * @return a const iterator that addresses the location succeeding
     * the last element in a string.
     */
    ConstPointer
    end() const throw ();

    /**
     * @return an iterator that addresses the location succeeding
     * the last element in a string.
     */
    Pointer
    end() throw ();

    /**
     * Removes a count number of elements in a SubString from front.
     * Actually moves the beginning of the substring forward on the specified
     * amount of elements leaving the end pointer intact.
     * @param count number of elements to be removed. NPOS value means
     * remove all.
     * @return reference on self.
     */
    BasicSubString&
    erase_front(SizeType count = NPOS) throw ();

    /**
     * Removes a count number of elements in a SubString from back.
     * Actually moves the end of the substring backward on the specified
     * amount of elements leaving the beginning pointer intact.
     * @param count number of elements to be removed. NPOS value means
     * remove all.
     * @return reference on self.
     */
    BasicSubString&
    erase_back(SizeType count = NPOS) throw ();

    // Finders
    /**
     * Searches a string in a forward direction for the first occurrence
     * of a specified character.
     * @param ch character to search
     * @param pos start position for search
     * @return position in SubString if the character found,
     * NPOS if not.
     */
    SizeType
    find(ValueType ch, SizeType pos = 0) const throw ();

#ifdef BASIC_STRING_INTERFACE_IMITATION
    SizeType
    find(ConstPointer ptr, SizeType pos = 0) const throw (LogicError);
#endif

    /**
     * Find position of a C substring.
     * Starting from pos, searches forward for the first count characters
     * in ptr within this SubString. If found, returns the index where it
     * begins.  If not found, returns NPOS.
     * @param ptr C string to locate.
     * @param pos Index of character to search from.
     * @param count Number of characters from ptr to search for.
     * @return Index of start of first occurrence.
     */
    SizeType
    find(ConstPointer ptr, SizeType pos, SizeType count) const
      throw (LogicError);

    /**
     * Find position of a SubString.
     * Starting from pos, searches forward for value of str within
     * this SubString. If found, returns the index where it begins.
     * If not found, returns NPOS.
     * @param str SubString to locate.
     * @param pos Index of character to search from (default 0).
     * @return Index of start of first occurrence.
     */
    SizeType
    find(const BasicSubString& str, SizeType pos = 0) const throw ();

#ifdef BASIC_STRING_INTERFACE_IMITATION
    /**
     * Searches through a string for the first character that is not
     * any element of a specified string.
     */
    SizeType
    find_first_not_of(ValueType ch, SizeType pos = 0) const;

    SizeType
    find_first_not_of(ConstPointer ptr, SizeType pos = 0) const;

    SizeType
    find_first_not_of(ConstPointer ptr, SizeType pos, SizeType count)
      const;

    SizeType
    find_first_not_of(const BasicSubString& str, SizeType pos = 0) const;

    /**
     * Searches through a string for the first character that matches
     * any element of a specified string.
     */
    SizeType
    find_first_of(ValueType ch, SizeType pos = 0) const;

    SizeType
    find_first_of(ConstPointer ptr, SizeType pos = 0) const;

    SizeType
    find_first_of(ConstPointer ptr, SizeType pos, SizeType count)
      const;

    SizeType
    find_first_of(const BasicSubString& str, SizeType pos = 0) const;

    /**
     * Searches through a string for the last character that is not any
     * element of a specified string.
     */
    SizeType
    find_last_not_of(ValueType ch, SizeType pos = NPOS) const;

    SizeType
    find_last_not_of(ConstPointer ptr, SizeType pos = NPOS) const;

    SizeType
    find_last_not_of(ConstPointer ptr, SizeType pos, SizeType count)
      const;

    SizeType
    find_last_not_of(const BasicSubString& str, SizeType pos = NPOS) const;

    /**
     * Searches through a string for the last character that is an
     * element of a specified string.
     */
    SizeType
    find_last_of(ValueType ch, SizeType pos = NPOS) const;

    SizeType
    find_last_of(ConstPointer ptr, SizeType pos = NPOS) const;

    SizeType
    find_last_of(ConstPointer ptr, SizeType pos, SizeType count) const;

    SizeType
    find_last_of(const BasicSubString& str, SizeType pos = NPOS) const;
#endif

    /**
     * @return length of SubString
     */
    SizeType
    length() const throw ();

    /**
     * @return the maximum number of characters a string could contain.
     */
    SizeType
    max_size() const throw ();

#ifdef BASIC_STRING_INTERFACE_IMITATION
    /**
     *  Returns an iterator to the first element in a reversed string.
     */
    ConstReverseIterator
    rbegin() const throw ();

    /**
     * non-const version
     * @return an iterator to the first element in a reversed string.
     */
    ReverseIterator
    rbegin() throw ();

    /**
     * Returns an iterator that points just beyond the last element
     * in a reversed string.
     */
    ConstReverseIterator
    rend() const throw ();

    ReverseIterator
    rend() throw ();

    /**
     * Replaces elements in a string at a specified position with specified
     * characters or characters copied from other ranges or strings
     * or C-strings.
     */
    BasicSubString&
    replace(SizeType pos1, SizeType count1, ConstPointer ptr) throw ();

    BasicSubString&
    replace(SizeType pos1, SizeType count1, const BasicSubString& str)
      throw ();

    BasicSubString&
    replace(SizeType pos1, SizeType count1, ConstPointer ptr,
      SizeType count2) throw ();

    BasicSubString&
    replace(SizeType pos1, SizeType count1, const BasicSubString& str,
      SizeType pos2, SizeType count2) throw ();

    BasicSubString&
    replace(SizeType pos1, SizeType count1, SizeType count, ValueType ch)
      throw ();

    BasicSubString&
    replace(Pointer first0, Pointer last0, ConstPointer ptr) throw ();

    BasicSubString&
    replace(Pointer first0, Pointer last0, const BasicSubString& str)
      throw ();

    BasicSubString&
    replace(Pointer first0, Pointer last0, ConstPointer ptr,
      SizeType count2) throw ();

    BasicSubString&
    replace(Pointer first0, Pointer last0, SizeType num2, ValueType ch)
      throw ();

    template <class InputIterator>
    BasicSubString&
    replace(Pointer first0, Pointer last0, InputIterator first,
      InputIterator last) throw ();

    /**
     * Specifies a new size for a string, appending or erasing elements
     * as required.
     */
    void
    resize(SizeType count) throw ();
#endif

    /**
     * Find last position of a character
     * Searches a string in a backward direction for the first occurrence
     * of a character.
     * @param ch character to locate.
     * @param pos index of character to search back from (default end).
     * @return index of last occurrence if found, NPOS if not found.
     */
    SizeType
    rfind(ValueType ch, SizeType pos = NPOS) const throw ();

#ifdef BASIC_STRING_INTERFACE_IMITATION
    SizeType
    rfind(ConstPointer ptr, SizeType pos = NPOS) const throw (LogicError);
#endif

    /**
     * Find last position of a C substring.
     * Starting from pos, searches backward for the first count
     * characters in ptr within this string. If found, returns the index
     * where it begins. If not found, returns NPOS.
     * @param ptr C string to locate.
     * @param pos Index of character to search back from.
     * @param count Number of characters from ptr to search for.
     * @return Index of start of last occurrence.
     */
    SizeType
    rfind(ConstPointer ptr, SizeType pos, SizeType count) const
      throw (LogicError);

    /**
     * Find last position of a SubString.
     * Starting from pos, searches backward for value of str within
     * this SubString. If found, returns the index where it begins. If not
     * found, returns NPOS.
     * @param str SubString to locate.
     * @param pos Index of character to search back from (default end).
     * @return Index of start of last occurrence.
     */
    SizeType
    rfind(const BasicSubString& str, SizeType pos = NPOS) const throw ();

    /**
     * @return the current number of elements in a SubString.
     */
    SizeType
    size() const throw ();

    /**
     * Copies a substring of at most some number of characters from
     * a string beginning from a specified position.
     * @param pos index of first character to store into BasicSubString.
     * @param count number of elements to store in BasicSubString.
     * @return BasicSubString object that coverage part of original
     * object.
     */
    BasicSubString
    substr(SizeType pos = 0, SizeType count = NPOS) const
      throw (OutOfRange);

    /**
     * Exchange the contents of two strings.
     * @param right object to exchange with *this.
     */
    void
    swap(BasicSubString& right) throw ();

    // Operators
    /**
     * Assign this SubString to point on right std::basic_string<...>
     * content.
     * @param str source std::basic_string<...>
     * @return reference on self
     */
    template <typename BasicStringTraits, typename Allocator>
    BasicSubString&
    operator =(const std::basic_string<BasicStringValueType,
      BasicStringTraits, Allocator>& str)
      throw(Exception);

    /**
     * Provides a const reference to the character with a specified
     * index in a SubString.
     * @param pos specified index in a SubString.
     * @return const reference to SubString character.
     */
    ConstReference
    operator [](SizeType pos) const throw (OutOfRange);

    /**
     * Provides a reference to the character with a specified
     * index in a SubString.
     * @param pos specified index in a SubString.
     * @return reference to SubString character.
     */
    Reference
    operator [](SizeType pos) throw (OutOfRange);

    /**
     * @return BasicString (i.e. std::basic_string<BasicStringValueType>)
     * object created on range
     */
    BasicString
    str() const throw ();

    /**
     * Assigns itself to std::string
     * @param str string to assign to
     */
    template <typename BasicStringTraits, typename Allocator>
    void
    assign_to(std::basic_string<BasicStringValueType, BasicStringTraits,
      Allocator>& str) const throw (Exception);

    /**
     * Append itself to the end of std::string
     * @param str string to append to
     */
    template <typename BasicStringTraits, typename Allocator>
    void
    append_to(std::basic_string<BasicStringValueType, BasicStringTraits,
      Allocator>& str) const throw (Exception);

  private:
    /**
     * @param pos specified index of begin sequence in a SubString.
     * @param count number of elements that asked for availability.
     * @return number of elements that available in string
     * with this pos and count.
     */
    SizeType
    get_available_length_(SizeType pos, SizeType count) const throw ();

    /**
     * Check awareness and shift begin pointer
     * @param position request shift in elements.
     * @param error_func used to create exception if incorrect
     * position requested.
     * @return shifted pointer on position
     */
    Pointer
    begin_plus_position_(SizeType position, const char* error_func) const
      throw (OutOfRange);

    Pointer begin_;
    SizeType length_;
  };

  /**
   * Auxiliary names and definitions for the implementation aims
   */
  namespace Helper
  {
    // This names of incomplete classes will shown in compile errors
    class ComparanceWithZeroPointerIsProhibited;
    class UseDefaultConstructorToCreateEmptySubString;

    /**
     * Short easy to use synonym of type
     * ComparanceWithZeroPointerIsProhibited
     */
    ComparanceWithZeroPointerIsProhibited
    pointers_case() throw ();

    /**
     * Short easy to use synonym of type
     * UseDefaultConstructorToCreateEmptySubString
     */
    UseDefaultConstructorToCreateEmptySubString
    constructor_case() throw ();

    /**
     * Will lead a compilation error with incomplete type T
     */
    template <typename T>
    void
    compiler_fail(T& p) throw ();
  }

  /**
   * Output the range to an ostream. Elements are outputted
   * in a sequence without separators.
   * @param ostr basic_ostream to out value of substr
   * @param substr SubString to be output
   * @return ostr.
   */

  template <typename CharType, typename Traits, typename Checker>
  std::basic_ostream<typename BasicSubString<CharType, Traits, Checker>::
    BasicStringValueType>&
  operator <<(std::basic_ostream<
    typename BasicSubString<CharType, Traits, Checker>::
      BasicStringValueType>& ostr,
    const BasicSubString<CharType, Traits, Checker>& substr)
    throw (Exception);

  ///////////////////////////////////////////////////////////////////////
  //Additional operators and methods
  /**
   * Comparison on equality
   * @param substr SubString to be compared
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @return true if equal values, false if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison on equality
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @param substr SubString to be compared
   * @return true if equal values, false if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(typename BasicSubString<CharType, Traits, Checker>::
    ConstPointer str, const BasicSubString<CharType, Traits, Checker>& substr)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison on equality with other SubString. Compare
   * memory entities.
   * @param left_substr left side of equality expression
   * @param right_substr SubString to compare with this
   * @return true if equal values, false if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr)
    throw ();

  /**
   * Comparison on equality with std::basic_string<...>. Compare
   * memory entities.
   * @param substr SubString to be compared
   * @param str BasicString to compare with this
   * @return true if equal values, false if not.
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    throw (Exception);

  /**
   * Comparison on equality with std::basic_string<...>. Compare
   * memory entities.
   * @param str BasicString to compare with this
   * @param substr SubString to be compared
   * @return true if equal values, false if not.
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator ==(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::BasicStringValueType,
      BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    throw (Exception);

  /**
   * The operator is defined to avoid error SubString a; if (0==a){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(
    int, const BasicSubString<CharType, Traits, Checker>&)
    throw ();

  /**
   * The operator is defined to avoid error SubString a; if (a==0){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(
    const BasicSubString<CharType, Traits, Checker>&, int)
    throw ();

  /**
   * Comparison on inequality
   * @param substr SubString to be compared
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @return false if equal values, true if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison on inequality
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @param substr SubString to be compared
   * @return false if equal values, true if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(typename BasicSubString<CharType, Traits, Checker>::
    ConstPointer str, const BasicSubString<CharType, Traits, Checker>& substr)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison two SubString on inequality. Compare
   * memory entities.
   * @param left_substr left side of expression
   * @param right_substr SubString to compare with this
   * @return false if equal values, true if not.
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr) throw ();

  /**
   * Comparison on inequality with std::basic_string<...>. Compare
   * memory entities.
   * @param substr SubString to be compared
   * @param str BasicString to compare with this
   * @return false if equal values, true if not.
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    throw (Exception);

  /**
   * Comparison on inequality with std::basic_string<...>. Compare
   * memory entities.
   * @param str BasicString to compare with this
   * @param substr SubString to be compared
   * @return false if equal values, true if not.
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator !=(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::BasicStringValueType,
      BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    throw (Exception);

  /**
   * The operator is defined to avoid error SubString a; if (0!=a){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(
    int, const BasicSubString<CharType, Traits, Checker>&)
    throw ();

  /**
   * The operator is defined to avoid error SubString a; if (a!=0){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator!=(
    const BasicSubString<CharType, Traits, Checker>&, int)
    throw();

  /**
   * Comparison on less
   * @param substr SubString to be compared
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @return false if substr >= str, else true
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison on less
   * @param str zero terminated C-string to compare its value
   * with SubString
   * @param substr SubString to be compared
   * @return false if substr >= str, else true
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(typename BasicSubString<CharType, Traits, Checker>::
    ConstPointer str, const BasicSubString<CharType, Traits, Checker>& substr)
    throw (typename BasicSubString<CharType, Traits, Checker>::LogicError);

  /**
   * Comparison two SubString on less. Compare
   * memory entities.
   * @param left_substr left side of expression
   * @param right_substr SubString to compare with this
   * @return false if left_substr >= right_substr, else true
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr) throw ();

  /**
   * Comparison on less
   * @param substr SubString to be compared
   * @param str std::string to compare with SubString
   * @return false if substr >= str, else true
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    throw ();

  /**
   * Comparison on less
   * @param str std::string to compare with SubString
   * @param substr SubString to be compared
   * @return false if str >= substr, else true
   */
  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator <(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::BasicStringValueType,
      BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    throw ();

  /**
   * The operator is defined to avoid error SubString a; if (0<a){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(
    int, const BasicSubString<CharType, Traits, Checker>&)
    throw ();

  /**
   * The operator is defined to avoid error SubString a; if (a<0){}
   * Now this code doesn't compile
   */
  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(
    const BasicSubString<CharType, Traits, Checker>&, int)
    throw ();

  typedef BasicSubString<const char, CharTraits<char>, CheckerNone<char> >
    SubString;

} /*Gears*/

#include "SubString.tpp"

#endif /*GEARS_SUBSTRING_HPP_*/
