#ifndef GEARS_COMPOSITE_ACTIVE_OBJECT_HPP
#define GEARS_COMPOSITE_ACTIVE_OBJECT_HPP

#include <algorithm>
#include <functional>
#include <deque>

#include "ActiveObject.hpp"
#include "OutputMemoryStream.hpp"

namespace Gears
{
  /**
   * class CompositeActiveObjectBase
   * This implements Active Object for control of several Active Objects.
   * Use this class to delegate active/inactive state control
   * over some set of Active Objects into one Active Object. In other words,
   * it usefully for central active/deactivate state management.
   * You SHOULD NOT change active/deactivate status of any added child
   * object into this CompositeActiveObject by yourself. Use it simply,
   * and activate or deactivate they through this holder. It will give
   * you guarantee of straight activation order, first added - first
   * activated, and guarantee for stopping object, last added, deactivate
   * and join first (reverse order). Optionally, you can switch off
   * stop ordering, it will be faster than consequently stopping and
   * safe for independent ActiveObjects (other words, use parallel stopping
   * if allow application logic). This behavior tune by the constructor
   * parameter sync_termination.
   * CompositeActiveObject is reference countable object.
   */
  template <typename Container>
  class CompositeActiveObjectBase: public SimpleActiveObject
  {
  public:
    DECLARE_EXCEPTION(ChildException, ActiveObject::Exception);
    DECLARE_EXCEPTION(CompositeAlreadyActive, ActiveObject::AlreadyActive);

    /**
     * Construct empty not active container for
     * ActiveObjects.
     * @param sync_termination true means do immediately wait after each
     * child Active Object deactivation.
     * @param clear_on_exit whether to call clear() in destructor or not
     */
    explicit
    CompositeActiveObjectBase(
      bool sync_termination = false,
      bool clear_on_exit = true)
      throw ();

    /**
     * Perform deactivating all owned objects, and waits for
     * its completion.
     */
    virtual
    ~CompositeActiveObjectBase() throw ();

    /**
     * Calls clear() for all owned objects
     */
    virtual
    void
    clear() throw (Gears::Exception);

    /**
     * Deactivate and wait for stop for all owned Active Objects.
     * Clears list of the objects.
     */
    void
    clear_children() throw (Exception, Gears::Exception);

    /**
     * This method fills CompositeActiveObject with other Active
     * objects. Control for consistent state, any time all object
     * that it own must be active or not active.
     * You delegate active state control to this container. Use
     * residual object only for work flow, do not change active status
     * through it.
     * @param child object that should go under the management of container.
     * @param add_to_head whether the object should be added to the head
     * of the list of contained objects or to the tail.
     */
    void
    add_child_object(const ActiveObject_var& child, bool add_to_head = false)
      throw (Exception, Gears::Exception);

  protected:
    // SimpleActiveObject interface
    /**
     * Activate all owned active objects. For empty case, simply change
     * status to Active. Throw CompositeAlreadyActive,
     * if you try to activate twice. All object activated successfully
     * or stay deactivated, if we were not able to activate any in the set.
     */
    virtual
    void
    activate_object_()
      throw (ActiveObject::Exception, Gears::Exception);

    /**
     * Deactivate (initiate stopping) all owned Active Objects.
     * Do nothing for empty and already deactivating object.
     * Perform deactivation as LIFO, last added Active Object
     * will start deactivation first.
     */
    virtual
    void
    deactivate_object_() throw (Exception, Gears::Exception);

    /**
     * Waits for deactivation all owned completion.
     * Perform waits as LIFO, last added Active Object will wait first.
     * That logic correspond deactivate_object method.
     */
    virtual
    void
    wait_object_() throw (Exception, Gears::Exception);

    /**
     * Simply calls wait_object for the given interval of objects
     */
    template <typename ReverseIterator>
    void
    wait_for_some_objects_(ReverseIterator rbegin, ReverseIterator rend)
      throw (Exception, Gears::Exception);

    /**
     * Thread-unsafe deactivation logic
     */
    void
    deactivate_object_(typename Container::reverse_iterator rit)
      throw (Exception, Gears::Exception);

  protected:
    const bool SYNCHRONOUS_;
    const bool CLEAR_ON_EXIT_;

    Container child_objects_;
  };

  /**
   * Default CompositeActiveObject containing a deque of ActiveObject_var.
   */
  typedef CompositeActiveObjectBase<
    std::deque<ActiveObject_var> >
    CompositeActiveObject;

  typedef std::shared_ptr<CompositeActiveObject>
    CompositeActiveObject_var;
}

namespace Gears
{
  template <typename Container>
  CompositeActiveObjectBase<Container>::CompositeActiveObjectBase(
    bool sync_termination, bool clear_on_exit) throw ()
    : SYNCHRONOUS_(sync_termination),
      CLEAR_ON_EXIT_(clear_on_exit)
  {}

  template <typename Container>
  CompositeActiveObjectBase<Container>::~CompositeActiveObjectBase()
    throw ()
  {
    if (CLEAR_ON_EXIT_)
    {
      try
      {
        clear();
      }
      catch (...)
      {}
    }

    try
    {
      clear_children();
    }
    catch (...)
    {}
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::activate_object_()
    throw (ActiveObject::Exception, Gears::Exception)
  {
    static const char* FUN = "CompositeActiveObjectBase::activate_object_()";

    typename Container::iterator it(child_objects_.begin());
    try
    {
      for (; it != child_objects_.end(); ++it)
      {
        (*it)->activate_object();
      }
    }
    catch (const Gears::Exception& e)
    {
      Gears::ErrorStream all_errors;
      try
      {
        state_ = AS_DEACTIVATING;
        typename Container::reverse_iterator rit(it);
        deactivate_object_(rit);
        wait_for_some_objects_(rit, child_objects_.rend());
      }
      catch (const Gears::Exception& e)
      {
        all_errors << e.what();
      }
      state_ = AS_NOT_ACTIVE;
      Gears::ErrorStream ostr;
      ostr << FUN << ": " << e.what();
      const Gears::SubString& all_errors_str = all_errors.str();
      if (all_errors_str.size())
      {
        ostr << all_errors_str;
      }
      throw ChildException(ostr.str());
    }
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::deactivate_object_()
    throw (Exception, Gears::Exception)
  {
    deactivate_object_(child_objects_.rbegin());
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::wait_object_()
    throw (Exception, Gears::Exception)
  {
    std::deque<ActiveObject_var> copy_of_child_objects;

    {
      Condition::Guard guard(cond_);
      for (typename Container::iterator itor = child_objects_.begin();
        itor != child_objects_.end(); ++itor)
      {
        copy_of_child_objects.push_back(*itor);
      }
    }

    wait_for_some_objects_(
      copy_of_child_objects.rbegin(),
      copy_of_child_objects.rend());
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::add_child_object(
    const ActiveObject_var& child, bool add_to_head)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "CompositeActiveObjectBase::add_child_object()";

    Condition::Guard guard(cond_);

    try
    {
      if(state_ == AS_ACTIVE)
      {
        if(!child->active())
        {
          child->activate_object();
        }
      }
      else
      {
        if (child->active())
        {
          child->deactivate_object();
          child->wait_object();
        }
      }

      std::inserter(
        child_objects_,
        add_to_head ? child_objects_.begin() : child_objects_.end()) =
        ActiveObject_var(child);
    }
    catch (const Gears::Exception& ex)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": Can't add object. Caught Gears::Exception: " <<
        ex.what();
      throw Exception(ostr.str());
    }
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::clear_children()
    throw (Exception, Gears::Exception)
  {
    Condition::Guard guard(cond_);

    if (state_ != AS_NOT_ACTIVE)
    {
      typename Container::reverse_iterator rit(child_objects_.rbegin());
      deactivate_object_(rit);
      wait_for_some_objects_(rit, child_objects_.rend());
      state_ = AS_NOT_ACTIVE;
    }
    child_objects_.clear();
  }

  template <typename Container>
  template <typename ReverseIterator>
  void
  CompositeActiveObjectBase<Container>::wait_for_some_objects_(
    ReverseIterator rit, ReverseIterator rend)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "CompositeActiveObjectBase::activate_object_()";

    Gears::ErrorStream all_errors;

    for (; rit != rend; ++rit)
    {
      try
      {
        (*rit)->wait_object();
      }
      catch (const Gears::Exception& ex)
      {
        all_errors << ex.what() << std::endl;
      }
    }

    const Gears::SubString& all_errors_str = all_errors.str();

    if (all_errors_str.size())
    {
      Gears::ErrorStream ostr;
      ostr << FUN <<
        ": Can't wait child active object. Caught Gears::Exception:\n";
      ostr << all_errors_str;
      throw Exception(ostr.str());
    }
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::deactivate_object_(
    typename Container::reverse_iterator rit)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "CompositeActiveObjectBase::deactivate_object_()";

    Gears::ErrorStream all_errors;

    for (; rit != child_objects_.rend(); ++rit)
    {
      try
      {
        (*rit)->deactivate_object();
        if (SYNCHRONOUS_)
        {
          (*rit)->wait_object();
        }
      }
      catch (const Gears::Exception& ex)
      {
        all_errors << ex.what() << std::endl;
      }
    }
    const Gears::SubString& all_errors_str = all_errors.str();
    if (all_errors_str.size())
    {
      Gears::ErrorStream ostr;
      ostr << FUN <<
        ": Can't deactivate child active object. Caught Gears::Exception:\n";
      ostr << all_errors_str;
      throw Exception(ostr.str());
    }
  }

  template <typename Container>
  void
  CompositeActiveObjectBase<Container>::clear() throw (Gears::Exception)
  {
    Condition::Guard guard(cond_);

    for (typename Container::iterator it(child_objects_.begin());
      it != child_objects_.end(); ++it)
    {
      (*it)->clear();
    }
  }
}

#endif
