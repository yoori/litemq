#include <iostream>
#include <unistd.h>

#include "SimpleLogger.hpp"

namespace
{
  const Gears::SubString SEVERITY_LABLES[] =
  {
    Gears::SubString("EMERGENCY"),
    Gears::SubString("ALERT"),
    Gears::SubString("CRITICAL"),
    Gears::SubString("ERROR"),
    Gears::SubString("WARNING"),
    Gears::SubString("NOTICE"),
    Gears::SubString("INFO"),
    Gears::SubString("DEBUG"),
    Gears::SubString("TRACE")
  };

  class Buffer
  {
  public:
    Buffer(char* buf, size_t length) throw (Gears::Exception);
    char*
    get() const throw ();
    size_t
    size() const throw ();
    void
    advance(size_t length) throw ();
    void
    advance() throw ();

  private:
    int size_;
    char* buff_ptr_;
  };

  inline
  Buffer::Buffer(char* buf, size_t length) throw (Gears::Exception)
    : size_(length), buff_ptr_(buf)
  {
    if (length >= 1)
    {
      *buff_ptr_ = '\0';
    }
  }

  inline
  char*
  Buffer::get() const throw ()
  {
    return size_ > 0 ? buff_ptr_: 0;
  }

  inline
  size_t
  Buffer::size() const throw ()
  {
    return size_;
  }

  inline
  void
  Buffer::advance(size_t length) throw ()
  {
    if (size_ > 0)
    {
      buff_ptr_ += length;
      size_ -= length;
    }
  }

  inline
  void
  Buffer::advance() throw ()
  {
    advance(size_ > 0 ? strlen(buff_ptr_) : 0);
  }
};

namespace Gears
{
  namespace Simple
  {
    //
    // Logger class
    //

    bool
    Logger::log(const Gears::SubString& text, unsigned long severity,
      const char* aspect, const char* code) throw ()
    {
      static const char* FUN = "Logger::log()";

      try
      {
        if (severity > static_cast<unsigned long>(log_level_))
        {
          return true;
        }

        Gears::Mutex::WriteGuard guard(lock_);

        if (!handler_)
        {
          Gears::ErrorStream ostr;
          ostr << FUN << ": handler is undefined";
          throw Exception(ostr.str());
        }

        LogRecord log_record;

        log_record.text = text;
        log_record.severity = severity;
        log_record.aspect = aspect ? Gears::SubString(aspect) :
          Gears::SubString();
        log_record.code = code ? Gears::SubString(code) :
          Gears::SubString();
        log_record.time = Gears::Time::get_time_of_day();
        log_record.time_zone = time_zone_;

        handler_->publish(log_record);
      }
      catch (const Gears::Exception& e)
      {
        if (error_stream_)
        {
          try
          {
            *error_stream_ << FUN << ": Gears::Exception caught:" << e.what();
          }
          catch (...)
          {
          }
        }
        return false;
      }

      return true;
    }


    //
    // Formatter class
    //

    size_t
    Formatter::required_size(const LogRecord& record) const
      throw (Exception, Gears::Exception)
    {
      return record.text.size() + record.aspect.size() + record.code.size() +
        1024;
    }

    bool
    Formatter::format(const LogRecord& record, char* buf, size_t size) const
      throw (Exception, Gears::Exception)
    {
      {
        size_t required_size = record.text.size() +
          (log_aspect_ ? record.aspect.size() : 0) +
          (log_code_ ? record.code.size() : 0) + 1024;
        if (required_size > size)
        {
          return false;
        }
      }

      Buffer buff(buf, size);

      if (log_time_)
      {
        const Gears::ExtendedTime& record_time(
          record.time.get_time(record.time_zone));

        strftime(buff.get(), buff.size(), "%a %d %b %Y", &record_time);
        buff.advance();

        snprintf(buff.get(), buff.size(), " %02d:%02d:%02d:%06d ",
          record_time.tm_hour, record_time.tm_min, record_time.tm_sec,
          record_time.tm_usec);
        buff.advance(17);
      }

      if (log_code_)
      {
        char* const BUFF = buff.get();
        const size_t RECORD_CODE_SIZE = record.code.size();
        assert(buff.size() >= RECORD_CODE_SIZE + 3);
        BUFF[0] = '[';
        memcpy(BUFF + 1, record.code.data(), RECORD_CODE_SIZE);
        BUFF[1 + RECORD_CODE_SIZE + 0] = ']';
        BUFF[1 + RECORD_CODE_SIZE + 1] = ' ';
        buff.advance(RECORD_CODE_SIZE + 3);
      }

      if (log_severity_)
      {
        const size_t SEVERITIES =
          sizeof(SEVERITY_LABLES) / sizeof(*SEVERITY_LABLES);

        const Gears::SubString& SEVERITY =
          SEVERITY_LABLES[record.severity < SEVERITIES ?
            record.severity : (SEVERITIES - 1)];

        char* BUFF = buff.get();
        const size_t SEVERITY_SIZE = SEVERITY.size();
        assert(buff.size() >= SEVERITY.size() + 4 + 20);
        BUFF[0] = '[';
        memcpy(BUFF + 1, SEVERITY.data(), SEVERITY_SIZE);
        if (record.severity >= SEVERITIES - 1)
        {
          buff.advance(SEVERITY_SIZE + 1);
          snprintf(buff.get(), buff.size(), " %lu] ",
            record.severity - (SEVERITIES - 1));
          buff.advance();
        }
        else
        {
          BUFF[1 + SEVERITY_SIZE + 0] = ']';
          BUFF[1 + SEVERITY_SIZE + 1] = ' ';
          buff.advance(SEVERITY_SIZE + 3);
        }
      }

      if (log_aspect_)
      {
        char* const BUFF = buff.get();
        const size_t RECORD_ASPECT_SIZE = record.aspect.size();
        assert(buff.size() >= RECORD_ASPECT_SIZE + 3);
        BUFF[0] = '[';
        memcpy(BUFF + 1, record.aspect.data(), RECORD_ASPECT_SIZE);
        BUFF[1 + RECORD_ASPECT_SIZE + 0] = ']';
        BUFF[1 + RECORD_ASPECT_SIZE + 1] = ' ';
        buff.advance(RECORD_ASPECT_SIZE + 3);
      }

      if (log_process_id_)
      {
        snprintf(buff.get(), buff.size(), "(%u) ",
          static_cast<unsigned>(getpid()));
        buff.advance();
      }

      if (log_thread_id_)
      {
        snprintf(buff.get(), buff.size(), "[%08lX] ",
          static_cast<unsigned long>(pthread_self()));
        buff.advance();
      }

      if (log_time_ || log_severity_ || log_aspect_ || log_thread_id_ ||
        log_process_id_)
      {
        assert(buff.size() >= 2);
        buff.get()[0] = ':';
        buff.get()[1] = ' ';
        buff.advance(2);
      }

      assert(buff.size() >= record.text.size() + 2);
      memcpy(buff.get(), record.text.data(), record.text.size());
      buff.advance(record.text.size());

      buff.get()[0] = '\n';
      buff.get()[1] = '\0';

      return true;
    }
  }


  Formatter_var
  FormatWrapper::create_default_formatter_() throw (Gears::Exception)
  {
    return Formatter_var(new Simple::Formatter);
  }
}
