#include "OutputMemoryStream.hpp"
#include "StreamLogger.hpp"

namespace Gears
{
  namespace OStream
  {
    namespace Helper
    {
      void
      Handler::publish(const LogRecord& record)
        throw (BadStream, Exception, Gears::Exception)
      {
        static const char* FUN = "OStream::Handler::publish()";

        FormatWrapper::Result line(formatter_.format(record));

        if (!line.get())
        {
          Gears::ErrorStream ostr;
          ostr << FUN << ": failed to format message";
          throw Exception(ostr.str());
        }

        ostr_ << line.get() << std::flush;

        if (!ostr_.good())
        {
          Gears::ErrorStream ostr;
          ostr << FUN << ": stream is dead";
          throw BadStream(ostr.str());
        }
      }
    }
  }
}
