#ifndef GEARS_DELEGATEACTIVEOBJECT_HPP_
#define GEARS_DELEGATEACTIVEOBJECT_HPP_

#include "ActiveObject.hpp"

namespace Gears
{
  /**
   * class DelegateActiveObject
   * simple wrapper for run thread
   * can be useful if you want control it with CompositeActiveObject (that contains other objects)
   */
  class DelegateActiveObject: public ActiveObjectCommonImpl
  {
  public:
    template<typename Delegate>
    DelegateActiveObject(
      const Delegate& delegate,
      const ActiveObjectCallback_var& callback,
      unsigned threads_number = 1,
      unsigned stack_size = 0)
      throw ()
      : ActiveObjectCommonImpl(
          SingleJob_var(new DelegateJob<Delegate>(delegate, callback)),
          threads_number,
          stack_size)
    {}

    virtual
    ~DelegateActiveObject() throw ()
    {
      std::cerr << "~DelegateActiveObject()" << std::endl;
    }

  private:
    template<typename Delegate>
    class DelegateJob: public SingleJob
    {
    public:
      DelegateJob(
        const Delegate& delegate,
        const ActiveObjectCallback_var& callback)
        throw()
        : SingleJob(callback),
          delegate_(delegate)
      {}

      virtual
      ~DelegateJob() throw () = default;

      virtual void
      work() throw()
      {
        while (!this->is_terminating())
        {
          delegate_();
        }
      }

      virtual void
      terminate() throw()
      {}

    private:
      Delegate delegate_;
    };

    class SelfDelegateJob : public SingleJob
    {
    public:
      SelfDelegateJob(
        DelegateActiveObject& delegate_active_object,
        const ActiveObjectCallback_var& callback)
        throw()
        : SingleJob(callback),
          delegate_active_object_(delegate_active_object)
      {}

      virtual
      ~SelfDelegateJob() throw () = default;

      virtual void
      work() throw()
      {
        delegate_active_object_.work_();
      }

      virtual void
      terminate() throw()
      {
        delegate_active_object_.terminate_();
      }

    private:
      DelegateActiveObject& delegate_active_object_;
    };

  protected:
    DelegateActiveObject(
      const ActiveObjectCallback_var& callback,
      unsigned threads_number = 1,
      unsigned stack_size = 0)
      throw ()
      : ActiveObjectCommonImpl(
          SingleJob_var(new SelfDelegateJob(*this, callback)),
          threads_number,
          stack_size)
    {}

    virtual void
    work_() throw ()
    {}

    virtual void
    terminate_() throw ()
    {}
  };

  typedef std::shared_ptr<DelegateActiveObject>
    DelegateActiveObject_var;

  template<typename Delegate>
  DelegateActiveObject_var
  make_delegate_active_object(
    const Delegate& delegate,
    const ActiveObjectCallback_var& callback,
    unsigned threads_number = 1)
    throw ()
  {
    return DelegateActiveObject_var(
      new DelegateActiveObject(
        delegate,
        callback,
        threads_number));
  }
}

#endif /* GEARS_DELEGATEACTIVEOBJECT_HPP_ */
