#ifndef GEARS_INPUTMEMORYSTREAM_HPP
#define GEARS_INPUTMEMORYSTREAM_HPP

#include <streambuf>
#include <istream>

#include "Exception.hpp"

/**
 * InputMemoryStreamBuffer
 * InputMemoryStream
 */
namespace Gears
{
  /**
   * Input memory buffer
   * Using supplied memory region as a stream content
   * No allocations are performed
   */
  template <typename Elem, typename Traits>
  class InputMemoryStreamBuffer : public std::basic_streambuf<Elem, Traits>
  {
  public:
    typedef typename Traits::int_type Int;
    typedef typename Traits::pos_type Position;
    typedef typename Traits::off_type Offset;

    typedef Elem* Pointer;
    typedef const Elem* ConstPointer;
    typedef size_t Size;

    /**
     * Constructor
     * @param ptr address of memory region
     * @param size size of memory region
     */
    InputMemoryStreamBuffer(Pointer ptr, Size size)
      throw(Gears::Exception);

    /**
     * @return The pointer to data not read yet
     */
    ConstPointer
    data() const throw();

    /**
     * @return The size of data not read yet
     */
    Size
    size() const throw();

  protected:
    virtual Position
    seekoff(
      Offset off,
      std::ios_base::seekdir way,
      std::ios_base::openmode which)
      throw(Gears::Exception);

    virtual Position
    seekpos(Position pos, std::ios_base::openmode which)
      throw(Gears::Exception);

    virtual Int
    underflow() throw();
  };

  /**
   * Input memory stream. Uses InputMemoryBuffer for data access.
   */
  template <typename Elem, typename Traits = std::char_traits<Elem> >
  class InputMemoryStream:
    public InputMemoryStreamBuffer<Elem, Traits>,
    public std::basic_istream<Elem, Traits>
  {
  private:
    typedef InputMemoryStreamBuffer<Elem, Traits> StreamBuffer;
    typedef std::basic_istream<Elem, Traits> Stream;

  public:
    typedef Elem* Pointer;
    typedef const Elem* ConstPointer;
    typedef size_t Size;

    /**
     * Constructor
     * Passes data and Traits::length(data) to InputMemoryBlock's
     * constructor
     * @param data address of memory region
     */
    InputMemoryStream(ConstPointer data)
      throw(Gears::Exception);

    /**
     * Constructor
     * Passes parameters to InputMemoryBlock's constructor
     * @param data address of memory region
     * @param size size of memory region
     */
    InputMemoryStream(ConstPointer data, Size size)
      throw(Gears::Exception);

    /**
     * Constructor
     * Passes str.data() and str.size() to InputMemoryBlock's constructor
     * @param str memory region, should not be temporal
     */
    template <typename Allocator>
    InputMemoryStream(const std::basic_string<Elem, Allocator>& str)
      throw(Gears::Exception);
  };
}

#include "InputMemoryStream.tpp"

#endif /*GEARS_INPUTMEMORYSTREAM_HPP*/
