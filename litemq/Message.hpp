#ifndef LITEMQ_MESSAGE_HPP_
#define LITEMQ_MESSAGE_HPP_

#include <vector>

namespace LiteMQ
{
  class MessageBuffer
  {
  public:
    MessageBuffer(std::vector<unsigned char>&& body);

    MessageBuffer(const void* buffer, unsigned long size);

    MessageBuffer(MessageBuffer&& init);

    const void*
    data() const throw();

    unsigned long
    size() const throw();

  protected:
    const std::vector<unsigned char> body_;
  };
}

namespace LiteMQ
{
  inline
  MessageBuffer::MessageBuffer(std::vector<unsigned char>&& body)
    : body_(std::move(body))
  {}

  inline
  MessageBuffer::MessageBuffer(
    const void* buffer,
    unsigned long size)
    : body_(
      static_cast<const unsigned char*>(buffer),
      static_cast<const unsigned char*>(buffer) + size)
  {}

  inline
  MessageBuffer::MessageBuffer(MessageBuffer&& init)
    : body_(std::move(init.body_))
  {}

  inline
  const void*
  MessageBuffer::data() const throw()
  {
    return &body_[0];
  }

  inline
  unsigned long
  MessageBuffer::size() const throw()
  {
    return body_.size();
  }
}

#endif /*LITEMQ_MESSAGE_HPP_*/
