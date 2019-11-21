#ifndef GEARS_OUTPUTMEMORYSTREAM_HPP
#define GEARS_OUTPUTMEMORYSTREAM_HPP

#include <streambuf>
#include <istream>
#include <ostream>

#include <sys/param.h>

#include "Exception.hpp"
#include "Allocator.hpp"
#include "SubString.hpp"

/**
 * OutputMemoryStreamBuffer
 * OutputMemoryStream
 * OutputBufferStream
 * OutputStackStream
 * ErrorStream
 * MemoryBufferHolder
 *
 * These classes are designed to be "less costly" versions of ostringstream
 * and istringstream in terms of allocations and copying.
 */
namespace Gears
{
  /**
   * Output memory buffer
   * Can preallocate memory region of desired size
   */
  template<
    typename Elem,
    typename Traits,
    typename Allocator,
    typename AllocatorInitializer = Allocator>
  class OutputMemoryStreamBuffer : public std::basic_streambuf<Elem, Traits>
  {
  public:
    typedef typename Traits::int_type Int;
    typedef typename Traits::pos_type Position;
    typedef typename Traits::off_type Offset;

    typedef typename Allocator::pointer Pointer;
    typedef typename Allocator::const_pointer ConstPointer;
    typedef typename Allocator::size_type Size;

    /**
     * Constructor
     * @param initial_size preallocated region size
     * @param allocator_initializer allocator initializer
     */
    OutputMemoryStreamBuffer(Size initial_size = 0,
      const AllocatorInitializer& allocator_initializer =
        AllocatorInitializer()) throw(Gears::Exception);

    /**
     * Destructor
     * Frees allocated memory region
     */
    virtual
    ~OutputMemoryStreamBuffer() throw();

    /**
     * @return The pointer to filled data
     */
    ConstPointer
    data() const throw();

    /**
     * @return The size of filled data
     */
    Size
    size() const throw();

  protected:
    virtual Position
    seekoff(Offset off,
      std::ios_base::seekdir way,
      std::ios_base::openmode which) throw(Gears::Exception);

    virtual Position
    seekpos(Position pos, std::ios_base::openmode which)
      throw(Gears::Exception);

    virtual Int
    overflow(Int c = Traits::eof()) throw(Gears::Exception);

  private:
    /**
     * Extends allocated memory region
     * @return whether or not extension was successful
     */
    bool
    extend() throw(Gears::Exception);

  private:
    Allocator allocator_;
    Offset max_offset_;
  };

  /**
   * Initializer of MemoryBuffer
   * Required because of order of construction
   */
  template <typename Buffer>
  class MemoryBufferHolder
  {
  public:
    typedef typename Buffer::char_type Elem;

    typedef typename Gears::BasicSubString<const Elem,
      CharTraits<typename RemoveConst<Elem>::Result>,
        CheckerNone<Elem> > SubString;

    /**
     * Constructor
     * Constructs memory buffer without parameters
     */
    MemoryBufferHolder() throw(Gears::Exception);

    /**
     * Constructor
     * Constructs memory buffer with one parameter
     * @param var parameter for buffer's constructor
     */
    template <typename T>
    MemoryBufferHolder(T var) throw(Gears::Exception);

    /**
     * Constructor
     * Constructs memory buffer with two parameters
     * @param var1 the first parameter for buffer's constructor
     * @param var2 the second parameter for buffer's constructor
     */
    template <typename T1, typename T2>
    MemoryBufferHolder(T1 var1, T2 var2) throw(Gears::Exception);

    /**
     * Return SubString based on this buffer. Buffer can be without
     * zero at the end.
     * @return SubString spreads the buffer
     */
    SubString
    str() const throw(Gears::Exception);

    /**
     * Templatized version of string which allow get SubString
     * with any suitable Traits and Checker types
     * @return SubString spreads the buffer
     */
    template <typename Traits, typename Checker>
    BasicSubString<const Elem, Traits, Checker>
    str() const throw(Gears::Exception);

  protected:
    /**
     * @return pointer to holding buffer
     */
    Buffer*
    buffer() throw();

    /**
     * @return pointer to holding buffer
     */
    const Buffer*
    buffer() const throw();

  private:
    Buffer buffer_;
  };

  /**
   * Output memory stream. Uses OutputMemoryStreamBuffer for data access.
   */
  template <
    typename Elem,
    typename Traits = std::char_traits<Elem>,
    typename Allocator = std::allocator<Elem>,
    typename AllocatorInitializer = Allocator, const size_t SIZE = 0>
  class OutputMemoryStream :
    public MemoryBufferHolder<
      OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer> >,
    public std::basic_ostream<Elem, Traits>
  {
  private:
    typedef MemoryBufferHolder<
      OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer> >
        Holder;
    typedef std::basic_ostream<Elem, Traits> Stream;

  public:
    /**
     * Constructor
     * Passes parameters to OutputMemoryBlock's constructor
     * @param initial_size preallocated region size
     * @param allocator_initializer allocator initializer
     */
    OutputMemoryStream(typename Allocator::size_type initial_size = SIZE,
      const AllocatorInitializer& allocator_initializer =
        AllocatorInitializer()) throw(Gears::Exception);
  };

  /**
   * Output memory stream working on external memory buffer of size
   * not less than SIZE. No more than SIZE-1 chars are written size_to
   * the buffer and buffer is always zero terminated after the
   * destruction of the stream.
   *
   * Example:
   * char buf[10];
   * {
   *   Buffer<5> ostr(buf);
   *   ostr << something;
   *   // buf IS NOT required to be nul-terminated here
   * }
   * // buf IS nul-terminated here. strlen(buf) <= 4.
   */
  template <const size_t SIZE>
  class OutputBufferStream: public OutputMemoryStream<
    char,
    std::char_traits<char>,
    FixedBufferAllocator<char, SIZE>,
    FixedBufferAllocator<char, SIZE>,
    SIZE - 1>
  {
  private:
    typedef FixedBufferAllocator<char, SIZE> Allocator;

  public:
    /**
     * Constructor
     * @param buffer buffer to make output to of size not less than SIZE
     */
    OutputBufferStream(char* buffer) throw();

    /**
     * Destructor
     * Appends nul-terminating character to the buffer
     */
    ~OutputBufferStream() throw();
  };

  /**
   * Output memory stream holding memory buffer of size SIZE+1.
   */
  template <const size_t SIZE>
  class OutputStackStream :
    public OutputMemoryStream<
      char,
      std::char_traits<char>,
      StackAllocator<char, SIZE + 1>,
      size_t,
      SIZE>
  {};

  /**
   * Default class for throwing DescriptiveException successors
   */
  class ErrorStream :
    public OutputStackStream<sizeof(Gears::DescriptiveException)>
  {};
} // Gears

#include "OutputMemoryStream.tpp"

#endif /*GEARS_OUTPUTMEMORYSTREAM_HPP*/
