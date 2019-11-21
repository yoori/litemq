#ifndef GEARS_OPTIONAL_HPP_
#define GEARS_OPTIONAL_HPP_

namespace Gears
{
  template<typename ObjectType>
  class Optional
  {
  public:
    Optional(): defined_(false), val_() {}

    template<typename...Args>
    explicit Optional(bool defined, Args... data): defined_(defined), val_(std::forward<Args>(data)...) {}

    explicit Optional(const ObjectType& val): defined_(true), val_(val) {}

    Optional(const ObjectType* val): defined_(val), val_(val ? *val : ObjectType())
    {}

    const ObjectType*
    operator->() const throw ()
    {
      return &val_;
    }

    ObjectType*
    operator->() throw ()
    {
      return &val_;
    }

    const ObjectType&
    operator*() const throw ()
    {
      return val_;
    }

    ObjectType&
    operator*() throw ()
    {
      return val_;
    }

    bool
    present() const throw ()
    {
      return defined_;
    }

    void
    set(const ObjectType& val)
    {
      val_ = val;
      defined_ = true;
    }

    ObjectType&
    fill()
    {
      defined_ = true;
      return val_;
    }

    Optional&
    operator=(const ObjectType& val)
    {
      set(val);
      return *this;
    }

    Optional&
    operator=(const Optional<ObjectType>& val)
    {
      if(val.present())
      {
        set(*val);
      }
      else
      {
        clear();
      }

      return *this;
    }

    template<typename RightType>
    Optional&
    operator=(const Optional<RightType>& val)
    {
      if(val.present())
      {
        set(*val);
      }
      else
      {
        clear();
      }

      return *this;
    }

    /*
    template<typename RightType>
    Optional&
    operator=(const RightType& val)
    {
      set(val);
      return *this;
    }
    */

    template<typename RightType>
    bool
    operator==(const Optional<RightType>& right) const
    {
      return &right == this || (present() == right.present() &&
        (!present() || **this == *right));
    }

    template <typename RightType>
    bool
    operator !=(const Optional<RightType>& right) const
    {
      return !(*this == right);
    }

    void
    clear()
    {
      defined_ = false;
      val_ = ObjectType();
    }

  protected:
    template <typename CompatibleType>
    Optional(const CompatibleType& val, bool defined)
      : defined_(defined), val_(defined ? ObjectType(val) : ObjectType())
    {}

    void
    present_(bool new_state) throw ()
    {
      defined_ = new_state;
    }

  private:
    bool defined_;
    ObjectType val_;
  };
}

#endif /*GEARS_OPTIONAL_HPP_*/
