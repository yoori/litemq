namespace Gears
{
  /* InputMemoryStreamBuffer impl */

  template<typename Elem, typename Traits>
  InputMemoryStreamBuffer<Elem, Traits>::InputMemoryStreamBuffer(
    Pointer ptr,
    Size size) throw(Gears::Exception)
  {
    this->setg(ptr, ptr, ptr + size);
  }

  template<typename Elem, typename Traits>
  typename InputMemoryStreamBuffer<Elem, Traits>::ConstPointer
  InputMemoryStreamBuffer<Elem, Traits>::data() const throw()
  {
    return this->gptr();
  }

  template<typename Elem, typename Traits>
  typename InputMemoryStreamBuffer<Elem, Traits>::Size
  InputMemoryStreamBuffer<Elem, Traits>::size() const throw()
  {
    return this->egptr() - this->gptr();
  }

  template<typename Elem, typename Traits>
  typename InputMemoryStreamBuffer<Elem, Traits>::Position
  InputMemoryStreamBuffer<Elem, Traits>::seekoff(
    Offset off,
    std::ios_base::seekdir way,
    std::ios_base::openmode which)
    throw(Gears::Exception)
  {
    if (which != std::ios_base::in)
    {
      return Position(Offset(-1)); // Standard requirements
    }

    Position pos(off);

    switch (way)
    {
    case std::ios_base::beg:
      break;

    case std::ios_base::cur:
      pos += this->gptr() - this->eback();
      break;

    case std::ios_base::end:
      pos = this->egptr() - this->eback() + pos;
      break;

    default:
      return Position(Offset(-1)); // Standard requirements
    }

    return seekpos(pos, which);
  }

  template<typename Elem, typename Traits>
  typename InputMemoryStreamBuffer<Elem, Traits>::Position
  InputMemoryStreamBuffer<Elem, Traits>::seekpos(
    Position pos,
    std::ios_base::openmode which)
    throw(Gears::Exception)
  {
    if (which != std::ios_base::in)
    {
      return Position(Offset(-1)); // Standard requirements
    }

    Offset offset(pos);

    if (offset < 0 || offset > this->egptr() - this->eback())
    {
      return Position(Offset(-1)); // Standard requirements
    }

    return pos;
  }

  template<typename Elem, typename Traits>
  typename InputMemoryStreamBuffer<Elem, Traits>::Int
  InputMemoryStreamBuffer<Elem, Traits>::underflow() throw()
  {
    return this->gptr() < this->egptr() ? *(this->gptr()) : Traits::eof();
  }

  /* InputMemoryStream impl */

  template<typename Elem, typename Traits>
  InputMemoryStream<Elem, Traits>::InputMemoryStream(ConstPointer data)
    throw(Gears::Exception)
    : StreamBuffer(const_cast<Pointer>(data), Traits::length(data)),
      Stream(this)
  {}

  template<typename Elem, typename Traits>
  InputMemoryStream<Elem, Traits>::InputMemoryStream(
    ConstPointer data, Size size) throw(Gears::Exception)
    : StreamBuffer(const_cast<Pointer>(data), size),
      Stream(this)
  {}

  template<typename Elem, typename Traits>
  template<typename Allocator>
  InputMemoryStream<Elem, Traits>::InputMemoryStream(
    const std::basic_string<Elem, Allocator>& str) throw(Gears::Exception)
    : StreamBuffer(const_cast<Pointer>(str.data()), str.size()),
      Stream(this)
  {}
}