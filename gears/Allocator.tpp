namespace Gears
{
  /* BasicFixedBufferAllocator impl */
  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
  BasicFixedBufferAllocator() throw()
    : allocated_(false)
  {
    buffer_[SIZE - 1] = '\0';
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
  BasicFixedBufferAllocator(
    BufferInitializer buffer_initializer) throw()
    : buffer_(buffer_initializer), allocated_(false)
  {
    buffer_[SIZE - 1] = '\0';
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  typename BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::
    Allocator::pointer
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::allocate(
    typename Allocator::size_type size, const void*) throw()
  {
    if (allocated_ || size >= SIZE)
    {
      return 0;
    }
    allocated_ = true;
    return buffer_;
  }

  template<typename Elem, const size_t SIZE, typename Buffer,
    typename BufferInitializer>
  void
  BasicFixedBufferAllocator<Elem, SIZE, Buffer, BufferInitializer>::deallocate(
    typename Allocator::pointer ptr, typename Allocator::size_type size)
    throw()
  {
    if (!allocated_ || ptr != buffer_ || size >= SIZE)
    {
      return;
    }
    allocated_ = false;
  }

  /* ArrayBuffer class */
  template<typename Elem, const size_t SIZE, typename Initializer>
  ArrayBuffer<Elem, SIZE, Initializer>::ArrayBuffer(
    Initializer /*initializer*/) throw()
  {}

  template<typename Elem, const size_t SIZE, typename Initializer>
  ArrayBuffer<Elem, SIZE, Initializer>::operator Elem*() throw()
  {
    return buffer_;
  }

  /* FixedBufferAllocator impl */

  template<typename Elem, const size_t SIZE>
  FixedBufferAllocator<Elem, SIZE>::FixedBufferAllocator(Elem* buffer) throw()
    : BasicFixedBufferAllocator<Elem, SIZE, Elem*>(buffer)
  {}

  /* StackAllocator impl */

  template<typename Elem, const size_t SIZE>
  StackAllocator<Elem, SIZE>::StackAllocator(size_t /*allocator_initializer*/)
    throw()
  {}
}
