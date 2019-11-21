namespace Gears
{
  /* OutputMemoryStreamBuffer impl */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    OutputMemoryStreamBuffer(Size initial_size,
    const AllocatorInitializer& allocator_initializer) throw(Gears::Exception)
    : allocator_(allocator_initializer),
      max_offset_(0)
  {
    Pointer ptr = 0;
    if (initial_size > 0)
    {
      ptr = allocator_.allocate(initial_size);
    }
    this->setp(ptr, ptr + initial_size);
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    ~OutputMemoryStreamBuffer() throw()
  {
    allocator_.deallocate(this->pbase(), this->epptr() - this->pbase());
    this->setp(0, 0);
    this->setg(0, 0, 0);
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  typename OutputMemoryStreamBuffer<Elem, Traits, Allocator,
    AllocatorInitializer>::ConstPointer
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    data() const throw()
  {
    return this->pbase();
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  typename OutputMemoryStreamBuffer<Elem, Traits, Allocator,
    AllocatorInitializer>::Size
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    size() const throw()
  {
    return this->pptr() - this->pbase();
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  typename OutputMemoryStreamBuffer<Elem, Traits, Allocator,
    AllocatorInitializer>::Position
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    seekoff(Offset off, std::ios_base::seekdir way,
    std::ios_base::openmode which) throw(Gears::Exception)
  {
    if (which != std::ios_base::out)
    {
      return Position(Offset(-1)); // Standard requirements
    }

    Offset current = this->pptr() - this->pbase();
    if (current > max_offset_)
    {
      max_offset_ = current;
    }

    Position pos(off);

    switch (way)
    {
    case std::ios_base::beg:
      break;

    case std::ios_base::cur:
      pos += current;
      break;

    case std::ios_base::end:
      pos = max_offset_ + pos;
      break;

    default:
      return Position(Offset(-1)); // Standard requirements
    }

    return seekpos(pos, which);
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  typename OutputMemoryStreamBuffer<Elem, Traits, Allocator,
    AllocatorInitializer>::Position
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    seekpos(Position pos, std::ios_base::openmode which)
    throw(Gears::Exception)
  {
    if (which != std::ios_base::out)
    {
      return Position(Offset(-1)); // Standard requirements
    }

    Offset current = this->pptr() - this->pbase();
    if (current > max_offset_)
    {
      max_offset_ = current;
    }

    Offset offset(pos);
    if (offset < 0 || offset > max_offset_)
    {
      return Position(Offset(-1)); // Standard requirements
    }
    this->setp(this->pbase(), this->epptr());
    this->pbump(static_cast<int>(offset));

    return pos;
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  bool
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    extend() throw(Gears::Exception)
  {
    Size old_size = this->epptr() - this->pbase();
    Offset offset = this->pptr() - this->pbase();

    Size new_size = 4096;
    while (new_size <= old_size)
    {
      new_size <<= 1;
      if (!new_size)
      {
        return false;
      }
    }

    Pointer ptr = allocator_.allocate(new_size);
    if (!ptr)
    {
      return false;
    }

    if (old_size > 0)
    {
      Traits::copy(ptr, this->pbase(), old_size);
      allocator_.deallocate(this->pbase(), old_size);
    }

    this->setp(ptr, ptr + new_size);
    this->pbump(static_cast<int>(offset));
    return true;
  }

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer>
  typename OutputMemoryStreamBuffer<Elem, Traits, Allocator,
    AllocatorInitializer>::Int
  OutputMemoryStreamBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
    overflow(Int c) throw(Gears::Exception)
  {
    Offset gpos = this->gptr() - this->eback();

    if (!extend())
    {
      return Traits::eof();
    }

    *this->pptr() = c;
    this->pbump(1);

    this->setg(this->pbase(), this->pbase() + gpos, this->pptr());
    return Traits::not_eof(c);
  }

  /* MemoryBufferHolder impl */

  template<typename Buffer>
  MemoryBufferHolder<Buffer>::MemoryBufferHolder() throw(Gears::Exception)
  {}

  template<typename Buffer>
  template<typename T>
  MemoryBufferHolder<Buffer>::MemoryBufferHolder(T arg)
    throw(Gears::Exception)
    : buffer_(arg)
  {}

  template<typename Buffer>
  template<typename T1, typename T2>
  MemoryBufferHolder<Buffer>::MemoryBufferHolder(T1 arg1, T2 arg2)
    throw(Gears::Exception)
    : buffer_(arg1, arg2)
  {}

  template<typename Buffer>
  typename MemoryBufferHolder<Buffer>::SubString
  MemoryBufferHolder<Buffer>::str() const throw(Gears::Exception)
  {
    return SubString(this->buffer()->data(),
      this->buffer()->size());
  }

  template<typename Buffer>
  template<typename Traits, typename Checker>
  BasicSubString<const typename MemoryBufferHolder<Buffer>::Elem,
    Traits, Checker>
  MemoryBufferHolder<Buffer>::str() const throw(Gears::Exception)
  {
    return BasicSubString<const Elem, Traits, Checker>(
      this->buffer()->data(),
      this->buffer()->size());
  }

  template<typename Buffer>
  Buffer*
  MemoryBufferHolder<Buffer>::buffer() throw()
  {
    return &buffer_;
  }

  template<typename Buffer>
  const Buffer*
  MemoryBufferHolder<Buffer>::buffer() const throw()
  {
    return &buffer_;
  }

  /* OutputMemoryStream impl */

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
    OutputMemoryStream(typename Allocator::size_type initial_size,
      const AllocatorInitializer& allocator_initializer)
    throw(Gears::Exception)
    : Holder(initial_size, allocator_initializer), Stream(this->buffer())
  {}

  /* Buffer class */

  template<const size_t SIZE>
  OutputBufferStream<SIZE>::OutputBufferStream(char* buffer) throw()
    : OutputMemoryStream<char, std::char_traits<char>,
        Allocator, Allocator, SIZE - 1>(SIZE - 1, Allocator(buffer))
  {}

  template<const size_t SIZE>
  OutputBufferStream<SIZE>::~OutputBufferStream() throw()
  {
    *this << '\0';
  }
}
