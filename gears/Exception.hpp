#ifndef BASIC_EXCEPTION_HPP
#define BASIC_EXCEPTION_HPP

#include <stdexcept>
#include <string>

#include <string.h>
#include <stdlib.h>

#define DECLARE_EXCEPTION(NAME, BASE)           \
  class NAME##_tag_; \
  typedef Gears::CompositeException<NAME##_tag_, BASE> NAME

namespace Gears
{
  template<typename CharType, typename Traits, typename Checker>
  class BasicSubString;

  /** The standard library exception. */
  typedef std::exception Exception;

  /**
   * A descriptive exception.
   * Extends std::exception to provides textual information describing
   * the exception raised.
   */
  class DescriptiveException : public virtual Exception
  {
  protected:
    explicit
    DescriptiveException(const char* description)
      throw();

    DescriptiveException(const char* description, size_t length)
      throw();

    explicit
    DescriptiveException(const std::string& description)
      throw();

    DescriptiveException(const DescriptiveException& exception)
      throw();

    template<typename CharType, typename Traits, typename Checker>
    DescriptiveException(
      const BasicSubString<CharType, Traits, Checker>& description)
      throw();

    DescriptiveException&
    operator=(const DescriptiveException& exception) throw();

  public:
    virtual ~DescriptiveException() throw();

    /** Returns the message associated with the exception. */
    virtual const char* what() const throw();

  protected:
    DescriptiveException() throw ();

    void
    copy_string_(const char* src, char* dst, size_t size) throw ();

    void
    init_(const char* description) throw ();

    void
    init_(const char* description, size_t length)
      throw ();

  protected:
    enum { DESC_EXCEPTION_BUFFER_SIZE = 10 * 1024 };
    char description_[DESC_EXCEPTION_BUFFER_SIZE];
  };

  template <typename Tag, typename Base>
  class CompositeException: public virtual Base
  {
  public:
    explicit
    CompositeException(const char* description) throw();

    CompositeException(const char* description, size_t length) throw();

    explicit
    CompositeException(const std::string& description)
      throw();

    template<typename CharType, typename Traits, typename Checker>
    CompositeException(
      const BasicSubString<CharType, Traits, Checker>& description)
      throw();

  protected:
    CompositeException() throw();
  };
}

namespace Gears
{
  /* DescriptiveException */
  inline
  DescriptiveException::DescriptiveException()
    throw()
  {
    init_(0, 0);
  }

  inline
  DescriptiveException::DescriptiveException(
    const char* description,
    size_t length)
    throw()
  {
    init_(description, length);
  }

  inline
  DescriptiveException::DescriptiveException(
    const std::string& description)
    throw()
  {
    init_(description.data(), description.size());
  }

  inline
  DescriptiveException::~DescriptiveException()
    throw()
  {
    memset(description_, 0, sizeof(description_));
  }

  inline
  DescriptiveException::DescriptiveException(
    const DescriptiveException& exception) throw ()
    : Exception()
  {
    init_(exception.description_);
  }

  template<typename CharType, typename Traits, typename Checker>
  DescriptiveException::DescriptiveException(
    const BasicSubString<CharType, Traits, Checker>& description)
    throw()
  {
    init_(description.data(), description.size());
  }

  inline
  DescriptiveException&
  DescriptiveException::operator=(
    const DescriptiveException& exception)
    throw()
  {
    init_(exception.description_);
    return *this;
  }

  inline
  const char*
  DescriptiveException::what() const throw ()
  {
    return description_;
  }

  inline
  void
  DescriptiveException::copy_string_(const char* src, char* dst, size_t size)
    throw()
  {
    if(src)
    {
      char* end = dst + size - 1;
      while ((*dst++ = *src++) && dst < end) {}
      *end = '\0';
    }
    else
    {
      *dst = '\0';
    }
  }

  inline
  void
  DescriptiveException::init_(const char* description)
    throw()
  {
    copy_string_(description, description_, DESC_EXCEPTION_BUFFER_SIZE);
  }

  inline
  void
  DescriptiveException::init_(const char* description, size_t size)
    throw()
  {
    if(size)
    {
      if(size > DESC_EXCEPTION_BUFFER_SIZE - 1)
      {
        size = DESC_EXCEPTION_BUFFER_SIZE - 1;
      }
      ::memcpy(description_, description, size);
    }

    description_[size] = '\0';
  }

  /* Composite */

  template<typename TagType, typename BaseType>
  CompositeException<TagType, BaseType>::CompositeException() throw ()
  {}

  template<typename TagType, typename BaseType>
  CompositeException<TagType, BaseType>::CompositeException(
    const char* description)
    throw()
  {
    BaseType::init_(description);
  }

  template<typename TagType, typename BaseType>
  CompositeException<TagType, BaseType>::CompositeException(
    const char* description, size_t size)
    throw()
  {
    BaseType::init_(description, size);
  }

  template<typename TagType, typename BaseType>
  CompositeException<TagType, BaseType>::CompositeException(
    const std::string& description)
    throw()
  {
    BaseType::init_(description.data(), description.size());
  }

  template<typename TagType, typename BaseType>
  template<typename CharType, typename Traits, typename Checker>
  CompositeException<TagType, BaseType>::CompositeException(
    const BasicSubString<CharType, Traits, Checker>& description)
    throw()
  {
    BaseType::init_(description.data(), description.size());
  }
}

#endif /*BASIC_EXCEPTION_HPP*/
