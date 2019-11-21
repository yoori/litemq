#include <iostream>

#include "ActiveObjectCallback.hpp"

namespace Gears
{
  //
  // ActiveObjectCallback class
  //

  void
  ActiveObjectCallbackImpl::report_error(Severity severity,
    const Gears::SubString& description,
    const char* error_code)
    throw ()
  {
    unsigned long log_level = 0;
    const char* str_severity = "UNKNOWN";

    switch (severity)
    {
    case CRITICAL_ERROR:
      log_level = Gears::Logger::EMERGENCY;
      str_severity = "CRITICAL_ERROR";
      break;

    case ERROR:
      log_level = Gears::Logger::CRITICAL;
      str_severity = "ERROR";
      break;

    case WARNING:
      log_level = Gears::Logger::WARNING;
      str_severity = "WARNING";
      break;
    }

    Gears::ErrorStream ostr;
    ostr << message_prefix() << " " << str_severity << "(" << severity <<
      ") report:" << description;

    Gears::Logger_var current_logger = logger();
    if (current_logger)
    {
      current_logger->log(ostr.str(), log_level, aspect(), code(error_code));
    }
    else
    {
      std::cerr << ostr.str() << std::endl;
    }
  }

  Logger_var
  ActiveObjectCallbackImpl::logger() const throw ()
  {
    return logger_;
  }

  const char*
  ActiveObjectCallbackImpl::message_prefix() const throw ()
  {
    return message_prefix_;
  }

  const char*
  ActiveObjectCallbackImpl::aspect() const throw ()
  {
    return aspect_;
  }

  const char*
  ActiveObjectCallbackImpl::code(const char* error_code) const throw ()
  {
    return error_code ? error_code : code_;
  }
}
