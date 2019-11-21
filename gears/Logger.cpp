#include "Logger.hpp"

namespace Gears
{
  const size_t StreamLogger::DEFAULT_BUFFER_SIZE;
  StreamLogger::ThreadBuffer StreamLogger::thread_buffer_;

  //
  // SimpleLoggerHolder class
  //

  SimpleLoggerHolder::SimpleLoggerHolder(Logger_var logger) throw ()
    : logger_(logger)
  {}

  unsigned long
  SimpleLoggerHolder::log_level() throw ()
  {
    return logger_->log_level();
  }

  void
  SimpleLoggerHolder::log_level(unsigned long value) throw ()
  {
    logger_->log_level(value);
  }

  bool
  SimpleLoggerHolder::log(const Gears::SubString& text,
    unsigned long severity, const char* aspect, const char* code) throw ()
  {
    return logger_->log(text, severity, aspect, code);
  }


  //
  // LoggerHolder class
  //

  LoggerHolder::LoggerHolder(Logger_var logger) throw ()
    : SimpleLoggerHolder(logger),
      log_level_(logger ? logger->log_level() : 0)
  {
  }

  void
  LoggerHolder::logger(Logger_var new_logger) throw ()
  {
    Logger_var nl(new_logger);
    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      std::swap(logger_, nl);
      log_level_ = logger_ ? logger_->log_level() : 0;
    }
  }

  unsigned long
  LoggerHolder::log_level() throw ()
  {
    return log_level_;
  }

  void
  LoggerHolder::log_level(unsigned long value) throw ()
  {
    Logger_var logger;
    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      if (!logger_)
      {
        return;
      }
      logger = logger_;
      log_level_ = value;
    }
    logger->log_level(value);
  }

  bool
  LoggerHolder::log(const Gears::SubString& text, unsigned long severity,
    const char* aspect, const char* code) throw ()
  {
    if (severity > static_cast<unsigned long>(log_level_))
    {
      return true;
    }

    Logger_var logger;
    {
      Gears::SpinLock::WriteGuard guard(mutex_);
      logger = logger_;
    }
    return logger ? logger->log(text, severity, aspect, code) : false;
  }


  //
  // LoggerDefaultHolder class
  //

  LoggerDefaultHolder::LoggerDefaultHolder(
    Logger_var logger,
    const char* aspect,
    const char* code) throw (Gears::Exception)
    : LoggerHolder(logger), aspect_(aspect ? aspect : ""),
      code_(code ? code : "")
  {}

  bool
  LoggerDefaultHolder::log(
    const Gears::SubString& text,
    unsigned long severity, const char* aspect, const char* code) throw ()
  {
    return LoggerHolder::log(text, severity,
      aspect ? aspect : aspect_.c_str(), code ? code : code_.c_str());
  }
}
