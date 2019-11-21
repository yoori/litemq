#ifndef GEARS_ACTIVE_OBJECT_HPP
#define GEARS_ACTIVE_OBJECT_HPP

//#include <Generics/Singleton.hpp>
#include "Lock.hpp"
#include "Condition.hpp"
#include "ThreadRunner.hpp"

namespace Gears
{
  /**
   * Reference countable callback for report errors.
   */
  class ActiveObjectCallback: public ThreadCallback
  {
  public:
    enum Severity
    {
      CRITICAL_ERROR = 0,
      ERROR = 1,
      WARNING = 2
    };

    virtual
    ~ActiveObjectCallback() throw () = default;

    virtual
    void
    report_error(
      Severity severity,
      const Gears::SubString& description,
      const char* error_code = 0) throw () = 0;

    void
    critical(const Gears::SubString& description,
      const char* error_code = 0) throw ();

    void
    error(const Gears::SubString& description,
      const char* error_code = 0) throw ();

    void
    warning(const Gears::SubString& description,
      const char* error_code = 0) throw ();
  };

  typedef std::shared_ptr<ActiveObjectCallback>
    ActiveObjectCallback_var;
  typedef std::shared_ptr<ActiveObjectCallback>
    FixedActiveObjectCallback_var;

  class ActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_EXCEPTION(NotSupported, Exception);
    DECLARE_EXCEPTION(AlreadyActive, Exception);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

  public:
    virtual
    void
    activate_object()
      throw (AlreadyActive, Exception, Gears::Exception) = 0;

    virtual
    void
    deactivate_object()
      throw (Exception, Gears::Exception) = 0;

    virtual
    void
    wait_object()
      throw (Exception, Gears::Exception) = 0;

    virtual
    bool
    active()
      throw (Gears::Exception) = 0;

    virtual
    void
    clear() throw (Gears::Exception);

  protected:
    virtual
    ~ActiveObject() throw () = default;

    enum ACTIVE_STATE
    {
      AS_ACTIVE,
      AS_DEACTIVATING,
      AS_NOT_ACTIVE
    };
  };

  typedef std::shared_ptr<ActiveObject> ActiveObject_var;

  /**
   * SimpleActiveObject implements expected ActiveObject state machine
   * and provides callbacks for additional state changing operations.
   * Base for many ActiveObjects.
   */
  class SimpleActiveObject: public virtual ActiveObject
  {
  public:
    SimpleActiveObject() throw (Gears::Exception);

    virtual
    void
    activate_object() throw (AlreadyActive, Exception, Gears::Exception);

    virtual
    void
    deactivate_object() throw (Exception, Gears::Exception);

    virtual
    void
    wait_object() throw (Exception, Gears::Exception);

    virtual
    bool
    active() throw (Gears::Exception);

  protected:
    virtual
    ~SimpleActiveObject() throw ();

    virtual
    void
    activate_object_() throw (Exception, Gears::Exception);

    virtual
    void
    deactivate_object_() throw (Exception, Gears::Exception);

    virtual
    bool
    wait_more_() throw (Exception, Gears::Exception);

    virtual
    void
    wait_object_() throw (Exception, Gears::Exception);

  protected:
    Condition cond_;
    volatile sig_atomic_t state_;
  };

  /**
   * General implementation Active Object logic by default.
   * May be supplement with special logic in concrete Active Object
   * through virtual methods override (of SingleJob descendand).
   */
  class ActiveObjectCommonImpl: public virtual ActiveObject
  {
  public:
    typedef ActiveObject::Exception Exception;
    typedef ActiveObject::NotSupported NotSupported;
    typedef ActiveObject::AlreadyActive AlreadyActive;
    typedef ActiveObject::InvalidArgument InvalidArgument;

    /**
     * Start threads that will perform SingleJob
     */
    virtual
    void
    activate_object()
      throw (AlreadyActive, Exception, Gears::Exception);

    /**
     * Initiate stopping of Active object
     * Acquires mutex and informs SingleJob
     */
    virtual
    void
    deactivate_object()
      throw (Exception, Gears::Exception);

    /**
     * Waits for deactivation completion
     * Acquires mutex and waits for threads completion
     */
    virtual
    void
    wait_object() throw (Exception, Gears::Exception);

    /**
     * Current status
     * @return Returns true if active and not going to deactivate
     */
    virtual
    bool
    active() throw (Gears::Exception);

  protected:
    /**
     * ActiveObjectCommonImpl expects the only object will be a job for
     * all ThreadRunner's threads. This object must be a descendant of
     * this class.
     */
    class SingleJob : public ThreadJob
    {
    public:
      typedef ActiveObject::Exception Exception;
      typedef ActiveObject::NotSupported NotSupported;
      typedef ActiveObject::AlreadyActive AlreadyActive;
      typedef ActiveObject::InvalidArgument InvalidArgument;

      /**
       * Constructor
       * @param callback callback to be called for error reporting
       */
      explicit
      SingleJob(const ActiveObjectCallback_var& callback)
        throw (InvalidArgument, Gears::Exception);

      virtual
      ~SingleJob() throw () = default;

      /**
       * Stored callback
       * @return stored callback
       */
      ActiveObjectCallback_var
      callback() throw ();

      /**
       * Mutex for operations synchronizations
       * @return stored mutex
       */
      Mutex&
      mutex() const throw ();

      virtual
      void
      started(unsigned threads) throw ();

      void
      make_terminate() throw ();

      void
      terminated() throw ();

      bool
      is_terminating() throw ();

      /**
       * Function must inform the object to stop jobs to work.
       */
      virtual
      void
      terminate() throw () = 0;

    private:
      mutable Mutex mutex_;
      ActiveObjectCallback_var callback_;
      volatile sig_atomic_t terminating_;
    };

    typedef std::shared_ptr<SingleJob> SingleJob_var;

    /**
     * Constructor
     * Initializes SINGLE_JOB_ with the provided job and
     * creates ThreadRunner.
     * @param job job to execute in threads
     * @param threads_number number of threads to execute the job in
     * @param stack_size stack size for threads
     * @param start_threads initial number of threads to start (0 - all)
     */
    explicit
    ActiveObjectCommonImpl(
      const SingleJob_var& job,
      unsigned threads_number = 1,
      size_t stack_size = 0,
      unsigned start_threads = 0)
      throw (InvalidArgument);

    /**
     * Destructor
     */
    virtual
    ~ActiveObjectCommonImpl() throw ();

    /**
     * @return the same mutex SINGLE_JOB_->mutex() returns
     */
    Mutex&
    mutex_() const throw ();

    SingleJob_var SINGLE_JOB_;
    ThreadRunner thread_runner_;

  private:
    unsigned start_threads_;

    mutable Mutex termination_mutex_;
    Mutex& work_mutex_;

    volatile sig_atomic_t active_state_;
  };
} // namespace Gears

// Inlines
namespace Gears
{
  //
  // ActiveObjectCallback class
  //
  inline
  void
  ActiveObjectCallback::critical(const Gears::SubString& description,
    const char* error_code)
    throw ()
  {
    report_error(CRITICAL_ERROR, description, error_code);
  }

  inline
  void
  ActiveObjectCallback::error(
    const Gears::SubString& description,
    const char* error_code)
    throw ()
  {
    report_error(ERROR, description, error_code);
  }

  inline
  void
  ActiveObjectCallback::warning(
    const Gears::SubString& description,
    const char* error_code)
    throw ()
  {
    report_error(WARNING, description, error_code);
  }

  //
  // SimpleActiveObject class
  //

  inline
  SimpleActiveObject::SimpleActiveObject() throw (Gears::Exception)
    : state_(AS_NOT_ACTIVE)
  {}

  //
  // class ActiveObjectCommonImpl
  //

  inline
  Mutex&
  ActiveObjectCommonImpl::mutex_() const throw ()
  {
    return work_mutex_;
  }

  //
  // ActiveObject class
  //

  inline
  void
  ActiveObject::clear() throw (Gears::Exception)
  {}

  //
  // ActiveObjectCommonImpl::SingleJob class
  //

  inline
  ActiveObjectCommonImpl::SingleJob::SingleJob(
    const ActiveObjectCallback_var& callback)
    throw (InvalidArgument, Gears::Exception)
    : callback_(callback),
      terminating_(false)
  {
    static const char* FUN = "ActiveObjectCommonImpl::SingleJob::SingleJob()";

    if (!callback)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": callback == 0";
      throw InvalidArgument(ostr.str());
    }
  }

  inline
  ActiveObjectCallback_var
  ActiveObjectCommonImpl::SingleJob::callback() throw ()
  {
    return callback_;
  }

  inline
  Mutex&
  ActiveObjectCommonImpl::SingleJob::mutex() const throw ()
  {
    return mutex_;
  }

  inline
  void
  ActiveObjectCommonImpl::SingleJob::started(unsigned /*threads*/) throw ()
  {}

  inline
  void
  ActiveObjectCommonImpl::SingleJob::make_terminate() throw ()
  {
    terminating_ = true;
    terminate();
  }

  inline
  void
  ActiveObjectCommonImpl::SingleJob::terminated() throw ()
  {
    terminating_ = false;
  }

  inline
  bool
  ActiveObjectCommonImpl::SingleJob::is_terminating() throw ()
  {
    return terminating_;
  }
}

#endif
