#ifndef LOGGER_STREAM_LOGGER_HPP
#define LOGGER_STREAM_LOGGER_HPP

#include "SimpleLogger.hpp"

namespace Gears
{
  namespace OStream
  {
    namespace Helper
    {
      /**
       * Configuration for OStream Handler
       */
      struct Config
      {
        /**
         * Constructor
         */
        Config(std::ostream& output_stream,
          Formatter_var formatter,
          size_t preallocated_size) throw ();

        std::ostream& output_stream;
        Formatter_var formatter;
        size_t preallocated_size;
      };

      /**
       * Writes formatted log line into the stream specified.
       */
      class Handler:
        public ::Gears::Handler
      {
      public:
        DECLARE_EXCEPTION(BadStream, Exception);

        /**
         * Constructor
         * @param config configuration
         */
        explicit
        Handler(Config&& config) throw (Gears::Exception);

        virtual
        ~Handler() throw() = default;

        /**
         * Writes record into stream.
         * @param record Specifies log record to publish.
         */
        virtual
        void
        publish(const LogRecord& record)
          throw (BadStream, Exception, Gears::Exception);

      protected:
        std::ostream& ostr_;
        FormatWrapper formatter_;
      };
    }

    struct Config :
      public Helper::Config,
      public Simple::Config
    {
      /**
       * Constructor
       * @param output_stream Stream to write to.
       * @param log_level Log level to be used for records filtering.
       * @param formatter Formatter to be used.
       * @param preallocated_size preallocated memory size for formatting
       */
      explicit
      Config(std::ostream& output_stream,
        unsigned long log_level = ::Gears::Logger::INFO,
        Formatter_var formatter = Formatter_var(),
        size_t preallocated_size = 0)
        throw ();
    };

    /**
     * Stream logger
     */
    typedef DerivedLogger<Config, Helper::Handler> Logger;
  }
}

//
// INLINES
//

namespace Gears
{
  namespace OStream
  {
    namespace Helper
    {
      //
      // Config class
      //

      inline
      Config::Config(std::ostream& output_stream,
        Formatter_var formatter_val,
        size_t preallocated_size) throw ()
        : output_stream(output_stream),
          formatter(formatter_val),
          preallocated_size(preallocated_size)
      {}

      //
      // Handler class
      //

      inline
      Handler::Handler(Config&& config) throw (Gears::Exception)
        : ostr_(config.output_stream),
          formatter_(config.formatter, config.preallocated_size)
      {
      }
    }


    //
    // Config class
    //

    inline
    Config::Config(std::ostream& output_stream,
      unsigned long log_level,
      Formatter_var formatter,
      size_t preallocated_size) throw ()
      : Helper::Config(output_stream, formatter, preallocated_size),
        Simple::Config(log_level)
    {}
  }
}

#endif
