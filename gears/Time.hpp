#ifndef GEARS_TIME_HPP
#define GEARS_TIME_HPP

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <iosfwd>
#include <limits>

#include "AsciiStringManip.hpp"
#include "OutputMemoryStream.hpp"

namespace Gears
{
  class ExtendedTime;

  /**
   * Time interval representation allowing microseconds granularity
   * Provides conversion to ExtendedTime
   *
   * tv_sec may be both positive and negative
   * tv_usec is between 0 and 1000000 (USEC_MAX) always
   * Total time in microseconds is (tv_sec * USEC_MAX + tv_usec)
   *
   * Another representation is
   * [{Print.sign}]{Print.integer_part}.{Print.fractional_part}.
   */
  struct Time : public timeval
  {
  public:
    DECLARE_EXCEPTION(Exception, Gears::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

    /**
     * Time zones types (either GMT or local)
     */
    enum TimeZone
    {
      TZ_GMT,
      TZ_LOCAL
    };

  public:
    /**
     * Prints out passed time value to the stream
     * @param tv Time value to print
     * @param ostr Stream for output
     */
    static
    void
    print(const timeval& tv, std::ostream& ostr)
      throw (Gears::Exception);

    /**
     * Finds month number (0-11) by its name
     * @param month 3-letter month name
     * @return Month index (0-11)
     */
    static
    unsigned
    month(const Gears::SubString& month)
      throw (InvalidArgument, Exception);

    /**
     * Gives month name by its number (0-11)
     * @param month index (0-11)
     * @return 3-letter month name
     */
    static
    const char*
    month(unsigned month)
      throw (InvalidArgument, Exception);

    /**
     * Finds week day index (0-6) by its name
     * @param day either a 3-letter or full week day name
     * @return week day index (Sunday first)
     */
    static
    unsigned
    week_day(const Gears::SubString& day)
      throw (InvalidArgument, Exception);

    /**
     * Gives week day name by its index
     * @param day index (0-6, Sunday first)
     * @return 3-letter week day name
     */
    static
    const char*
    week_day(unsigned day)
      throw (InvalidArgument, Exception);

    /**
     * Compares two tm dates. No TZ check is performed
     * @param t1 the first date
     * @param t2 the second date
     * @return Negative if less, zero if equal, positive if greater
     */
    static
    int
    compare(const tm& t1, const tm& t2) throw ();

    /**
     * Creates Time object holding current time value
     * @return current time
     */
    static
    Time
    get_time_of_day() throw ();


  public:
    /**
     * Default constructor
     * Initializes structure with zeros.
     */
    Time() throw ();

    /**
     * Constructor
     * @param time provided time
     */
    explicit
    Time(const timeval& time) throw ();

    /**
     * Constructor
     * @param time_sec provided time (seconds)
     * @param usec provided time (microseconds)
     */
    explicit
    Time(time_t time_sec, suseconds_t usec = 0) throw ();

    /**
     * Constructor
     * Parameters are equivalent to those in set function
     * @param value time string
     * @param format format string
     * @param strict either all digits must present or not
     */
    Time(const Gears::SubString& value, const char* format,
      bool strict = false)
      throw (InvalidArgument, Exception, Gears::Exception);

    /**
     * Converter to ExtendedTime
     * @param tz required TZ
     * @return ExtendedTime presentation based on current value and required tz
     */
    ExtendedTime
    get_time(TimeZone tz) const
      throw (Exception, Gears::Exception);

    /**
     * Converter to ExtendedTime
     * @return ExtendedTime GMT presentation based on current value
     */
    ExtendedTime
    get_gm_time() const throw (Exception, Gears::Exception);
    /**
     * Converter to ExtendedTime
     * @return ExtendedTime localtime presentation based on current value
     */
    ExtendedTime
    get_local_time() const throw (Exception, Gears::Exception);


    /**
     * Resets current value
     * @param time_sec seconds
     * @param usec microseconds
     */
    void
    set(time_t time_sec, suseconds_t usec = 0) throw ();


    /**
     * Parses string time representation.
     * Format resembles strptime()'s one using %q for microseconds.
     * @param value time string
     * @param format format string
     * @param strict either all digits must present or not
     */
    void
    set(const Gears::SubString& value, const char* format,
      bool strict = false)
      throw (InvalidArgument, Exception, Gears::Exception);


    struct Print
    {
      // -1, 0 or 1
      int sign;
      // non-negative
      time_t integer_part;
      // non-negative
      suseconds_t fractional_part;
    };

    /**
     * For printing only:
     * [{Print.sign}]{Print.integer_part}.{Print.fractional_part}
     * Returns structure for printing
     * @return Print structure
     */
    Print
    print() const throw ();

    /**
     * Inverts sign of the time interval
     */
    void
    invert_sign() throw ();

    /**
     * Returns tv_sec * USEC_MAX + tv_usec
     * @return microseconds representation of time value
     */
    long long
    microseconds() const throw ();

    /**
     * Returns imprecise double converted value.
     * Good for rough operations.
     * @return floating point presentation of the value
     */
    double
    as_double() const throw ();

    /**
     * Packs current value into TIME_PACK_LEN bytes long buffer
     * @param buffer pointer to TIME_PACK_LEN bytes long buffer
     */
    void
    pack(void* buffer) const throw ();

    /**
     * Unpacks current value from TIME_PACK_LEN bytes long buffer
     * @param buffer pointer to TIME_PACK_LEN bytes long buffer
     */
    void
    unpack(const void* buffer) throw ();


    /**
     * Adds another time interval to the current
     * @param time time interval to add
     * @return reference to the object
     */
    Time&
    operator +=(const Time& time) throw ();

    /**
     * Adds another time interval to the current
     * @param time time interval (seconds) to add
     * @return reference to the object
     */
    Time&
    operator +=(time_t time) throw ();

    /**
     * Subtracts another time interval from the current
     * @param time time interval to subtract
     * @return reference to the object
     */
    Time&
    operator -=(const Time& time) throw ();

    /**
     * Subtracts another time interval from the current
     * @param time time interval (seconds) to subtract
     * @return reference to the object
     */
    Time&
    operator -=(time_t time) throw ();

    /**
     * Multiplies current time interval on non-negative integer multiplier
     * @param multiplier multiplier
     * @return reference to the object
     */
    Time&
    operator *=(int multiplier) throw ();

    /**
     * Divides current time interval on non-negative integer divisor
     * @param divisor divisor
     * @return reference to the object
     */
    Time&
    operator /=(int divisor) throw ();



    /**
     * Quicker way to call get_gm_time().format("%F")
     * @return formatted GM time
     */
    std::string
    gm_f() const throw (Gears::Exception);

    /**
     * Quicker way to call get_gm_time().format("%F %T")
     * @return formatted GM time
     */
    std::string
    gm_ft() const throw (Gears::Exception);

    /**
     * Quicker way to call set(value, "%Y-%m-%d", strict).
     * @param value time string
     * @param strict if true leading zeros must present
     */
    void
    set_f(const Gears::SubString& value, bool strict = false)
      throw (InvalidArgument, Exception, Gears::Exception);

    /**
     * Quicker way to call set(value, "%Y-%m-%d %H:%M:%S", strict).
     * @param value time string
     * @param strict if true leading zeros must present
     */
    void
    set_ft(const Gears::SubString& value, bool strict = false)
      throw (InvalidArgument, Exception, Gears::Exception);

  public:
    static const std::size_t TIME_PACK_LEN = 8;

    static const unsigned long TIME_LEN = 21;

    static const suseconds_t USEC_MAX = 1000000;

    static const Time ZERO;
    static const Time ONE_SECOND;
    static const Time ONE_MINUTE;
    static const Time ONE_HOUR;
    static const Time ONE_DAY;
    static const Time ONE_WEEK;
  };

  /**
   * Representation of divided time according to selected TZ
   * Microseconds granularity is provided
   * Could be converted into Time object
   */
  class ExtendedTime : public tm
  {
  public:
    int tm_usec;
    Time::TimeZone timezone;

  public:
    typedef Time::Exception Exception;
    DECLARE_EXCEPTION(InvalidArgument, Exception);

    /**
     * Constructor
     * @param time divided time
     * @param usec microseconds
     * @param tz TZ for supplied time
     */
    ExtendedTime(const tm& time, suseconds_t usec, Time::TimeZone tz)
      throw ();
    /**
     * Constructor
     * @param sec seconds from Epoch
     * @param usec microseconds
     * @param tz TZ to convert to
     */
    ExtendedTime(time_t sec, suseconds_t usec, Time::TimeZone tz)
      throw (Exception, Gears::Exception);

    /**
     * Constructor
     * GMT is assumed
     * @param year year (A.D.)
     * @param month month index (1-12)
     * @param day day number (1-31)
     * @param hour hours (0-23)
     * @param min minutes (0-59)
     * @param sec seconds (0-59)
     * @param usec microseconds
     */
    ExtendedTime(
      int year,
      int month,
      int day,
      int hour,
      int min,
      int sec,
      suseconds_t usec) throw ();

    /**
     * Time conversion operator
     * @return Time object representing current value
     */
    operator Time() const
      throw (Exception, Gears::Exception);

    /**
     * Formats time represented by this object according to fmt.
     * Format resembles strftime()'s one using %q for microseconds.
     * @param fmt format string.
     * @return formatted time string
     */
    std::string
    format(const char* fmt) const
      throw (InvalidArgument, Exception, Gears::Exception);

    /**
     * Provides time normalization (i.e. 32nd of October becomes
     * 1st of November)
     */
    void
    normalize() throw (Exception, Gears::Exception);

    /**
     * Gives time part of the current value
     * @return time part of the current value (date fields are zeroed)
     */
    ExtendedTime
    get_time() const throw (Gears::Exception);

    /**
     * Copies time part of supplied value
     * @param time time to copy
     */
    void
    set_time(const ExtendedTime& time) throw ();

    /**
     * Gives date part of the current value
     * @return date part of the current value (time fields are zeroed)
     */
    ExtendedTime
    get_date() const throw (Gears::Exception);

    /**
     * Copies date part of supplied value
     * @param time date to copy
     */
    void
    set_date(const ExtendedTime& time) throw ();

  protected:
    static const Gears::Ascii::Caseless DAYS_[];
    static const Gears::Ascii::Caseless DAYS_FULL_[];
    static const Gears::Ascii::Caseless MONTHS_[];
    static const Gears::Ascii::Caseless MONTHS_FULL_[];

    const char*
    from_str_(const Gears::SubString& value, const char* format,
      bool strict) throw ();
    size_t
    to_str_(char* str, size_t length, const char* format) const
      throw ();

    friend struct Time;
  };

  /**
   * timegm(3) analogue
   * @param et split time stamp
   * @return seconds since epoch
   */
  time_t
  gm_to_time(const tm& et) throw ();

  /**
   * gmtime_r(3) analogue
   * @param time seconds since epoch to split
   * @param et resulted split time
   */
  void
  time_to_gm(time_t time, tm& et) throw ();

  template <typename Hash>
  void
  hash_add(Hash& hash, const Time& key) throw ();

  /**
   * Timer allows to calculate time intervals between two points in time
   * The first is marked to start() call and the second is marked by stop()
   */
  template <typename TimeStamp, typename Clock>
  class GeneralTimer
  {
  public:
    /**
     * Constructor
     */
    GeneralTimer() throw ();

    /**
     * Mark the first time point
     */
    void
    start() throw ();

    /**
     * Mark the second time point
     */
    void
    stop() throw ();

    /**
     * Mark the second time point and set time to elapsed time
     * @param timeout Will be assigned elapsed time value
     */
    void
    stop_set(TimeStamp& timeout) throw ();

    /**
     * Mark the second time point and add elapsed time to timeout.
     * @param timeout Elapsed time will be added to it
     */
    void
    stop_add(TimeStamp& timeout) throw ();

    /**
     * Start timestamp
     * @return start time stamp
     */
    TimeStamp
    start_time() const throw ();

    /**
     * Stop timestamp
     * @return start time stamp
     */
    TimeStamp
    stop_time() const throw ();

    /**
     * Calculate a difference between the second and the first time points
     * @return time interval
     */
    TimeStamp
    elapsed_time() const throw ();

  private:
    Clock clock_;
    bool started_;
    Time start_;
    Time stop_;
  };

  class ClockTimeOfDay
  {
  public:
    Time
    operator ()() const throw ();
  };

  /**
   * Timer allows to calculate time intervals between two points in real time
   * The first is marked to start() call and the second is marked by stop()
   */
  typedef GeneralTimer<Time, ClockTimeOfDay> Timer;

  /**
   * TimeMeter class is a guard that start/stop timer, which is based
   * and store the elapsed time
   * @param Timer The basis for the measurement of time
   * @param ADDITIVE The guard adds timeout to result, if true, simply
   * save the result if false.
   */
  template <typename Timer = GeneralTimer<Time, ClockTimeOfDay>, const bool ADDITIVE = false>
  class TimeMeter : private Timer
  {
  public:
    /**
     * Start timer
     * @param timeout Save resulting elapsed time
     */
    explicit
    TimeMeter(Time& timeout) throw ();

    /**
     * Stop timer and set elapsed time
     */
    ~TimeMeter() throw ();
  private:
    Gears::Time& time_;
  };

  typedef TimeMeter<Timer, false> ScopedTimer;
  typedef TimeMeter<Timer, true> ScopedAddTimer;
}

// Comparison functions (for Gears::Time class)
bool
operator ==(const timeval& tv1, const timeval& tv2) throw ();

bool
operator !=(const timeval& tv1, const timeval& tv2) throw ();

bool
operator <(const timeval& tv1, const timeval& tv2) throw ();

bool
operator >(const timeval& tv1, const timeval& tv2) throw ();

bool
operator <=(const timeval& tv1, const timeval& tv2) throw ();

bool
operator >=(const timeval& tv1, const timeval& tv2) throw ();

// Arithmetics functions (for Gears::Time class)
Gears::Time
operator -(const timeval& time) throw ();

Gears::Time
operator +(const timeval& tv1, const timeval& tv2) throw ();

Gears::Time
operator +(const timeval& tv, time_t time) throw ();

Gears::Time
operator -(const timeval& tv1, const timeval& tv2) throw ();

Gears::Time
operator -(const timeval& tv, time_t time) throw ();

Gears::Time
operator *(const timeval& tv, int multiplier) throw ();

Gears::Time
operator /(const timeval& tv, int divisor) throw ();

// Stream functions
std::ostream&
operator <<(std::ostream& ostr, const Gears::Time& time)
  throw (Gears::Exception);

std::ostream&
operator <<(std::ostream& ostr, const Gears::ExtendedTime& time)
  throw (Gears::Exception);

// Inlines
namespace Gears
{
  //
  // ExtendedTime class
  //

  inline
  ExtendedTime::ExtendedTime(
    const tm& time,
    suseconds_t usec,
    Time::TimeZone tz) throw ()
    : tm(time), tm_usec(usec), timezone(tz)
  {}

  inline
  ExtendedTime::ExtendedTime(
    int year,
    int month,
    int day,
    int hour,
    int min,
    int sec,
    suseconds_t usec) throw ()
  {
    tm_year = year - 1900;
    tm_mon = month - 1;
    tm_mday = day;
    tm_hour = hour;
    tm_min = min;
    tm_sec = sec;
    tm_usec = usec;
    timezone = Time::TZ_GMT;

    time_to_gm(gm_to_time(*this), *this);
  }

  inline
  ExtendedTime::operator Time() const
    throw (Exception, Gears::Exception)
  {
    time_t sec = 0;
    switch (timezone)
    {
    case Time::TZ_LOCAL:
      {
        tm tmp = *this;
        sec = ::mktime(&tmp);
      }
      break;

    case Time::TZ_GMT:
      sec = gm_to_time(*this);
      break;
    }

    return Time(sec, tm_usec);
  }

  inline void
  ExtendedTime::normalize() throw (Exception, Gears::Exception)
  {
    static const char* FUN = "ExtendedTime::normalize()";

    const time_t invalid = static_cast<time_t>(-1);
    time_t res = invalid;
    switch (timezone)
    {
    case Time::TZ_GMT:
      time_to_gm(gm_to_time(*this), *this);
      res = 0;
      break;
    case Time::TZ_LOCAL:
      res = mktime(this);
      break;
    default:
      break;
    }

    if(res == invalid)
    {
      ErrorStream ostr;
      ostr << FUN << ": can't normalize.";
      throw Exception(ostr.str());
    }
  }

  inline ExtendedTime
  ExtendedTime::get_time() const throw (Gears::Exception)
  {
    ExtendedTime res(*this);
    res.tm_mday = 0;
    res.tm_mon = 0;
    res.tm_wday = 0;
    res.tm_yday = 0;
    res.tm_year = 0;
    return res;
  }

  inline void
  ExtendedTime::set_time(const ExtendedTime& time) throw ()
  {
    tm_hour = time.tm_hour;
    tm_min = time.tm_min;
    tm_sec = time.tm_sec;
    tm_usec = time.tm_usec;
  }

  inline ExtendedTime
  ExtendedTime::get_date() const throw (Gears::Exception)
  {
    ExtendedTime res(*this);
    res.tm_hour = 0;
    res.tm_min = 0;
    res.tm_sec = 0;
    res.tm_usec = 0;
    return res;
  }

  inline void
  ExtendedTime::set_date(const ExtendedTime& time) throw ()
  {
    tm_mday = time.tm_mday;
    tm_mon = time.tm_mon;
    tm_year = time.tm_year;
  }


  //
  // Time class
  //

  inline Time
  Time::get_time_of_day() throw ()
  {
    Time time;
    gettimeofday(&time, 0);
    return time;
  }

  inline
  Time::Time() throw ()
#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  {
    tv_sec = 0;
    tv_usec = 0;
  }
#else
    : timeval{0, 0}
  {}
#endif

  inline
  Time::Time(const timeval& time) throw ()
    : timeval(time)
  {}

  inline
  Time::Time(time_t time_sec, suseconds_t usec) throw ()
#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  {
    tv_sec = time_sec;
    tv_usec = usec;
  }
#else
    : timeval{time_sec, usec}
  {
  }
#endif
}

// Arithmetic functions

inline Gears::Time
operator -(const timeval& tv) throw ()
{
  return tv.tv_usec ? Gears::Time(-tv.tv_sec - 1,
    Gears::Time::USEC_MAX - tv.tv_usec) : Gears::Time(-tv.tv_sec, 0);
}

inline Gears::Time
abs(const timeval& tv) throw ()
{
  return tv.tv_sec < 0 ? -Gears::Time(tv) : Gears::Time(tv);
}

inline Gears::Time
operator +(const timeval& tv1, const timeval& tv2) throw ()
{
  return tv1.tv_usec + tv2.tv_usec >= Gears::Time::USEC_MAX ?
    Gears::Time(tv1.tv_sec + tv2.tv_sec + 1,
      tv1.tv_usec + tv2.tv_usec - Gears::Time::USEC_MAX) :
    Gears::Time(tv1.tv_sec + tv2.tv_sec, tv1.tv_usec + tv2.tv_usec);
}

inline Gears::Time
operator +(const timeval& tv, time_t time) throw ()
{
  return Gears::Time(tv.tv_sec + time, tv.tv_usec);
}

inline Gears::Time
operator -(const timeval& tv1, const timeval& tv2) throw ()
{
  return tv1.tv_usec < tv2.tv_usec ?
    Gears::Time(tv1.tv_sec - tv2.tv_sec - 1,
      Gears::Time::USEC_MAX + tv1.tv_usec - tv2.tv_usec) :
    Gears::Time(tv1.tv_sec - tv2.tv_sec, tv1.tv_usec - tv2.tv_usec);
}

inline Gears::Time
operator -(const timeval& tv, time_t time) throw ()
{
  return Gears::Time(tv.tv_sec - time, tv.tv_usec);
}

namespace Helper
{
  inline int
  abs(int value) throw ()
  {
    return value < 0 ? -value : value;
  }

  inline Gears::Time
  mul(const timeval& tv, int multiplier) throw ()
  {
    return Gears::Time(tv.tv_sec * multiplier +
      static_cast<time_t>(tv.tv_usec) * multiplier /
        Gears::Time::USEC_MAX,
      static_cast<suseconds_t>(static_cast<time_t>(tv.tv_usec) *
        multiplier % Gears::Time::USEC_MAX));
  }

  inline Gears::Time
  div(const timeval& tv, int divisor) throw ()
  {
    return Gears::Time(tv.tv_sec / divisor,
      static_cast<suseconds_t>((
        (tv.tv_sec - tv.tv_sec / divisor * divisor) *
          Gears::Time::USEC_MAX + tv.tv_usec) / divisor));
  }
}

inline Gears::Time
operator *(const timeval& tv, int multiplier) throw ()
{
  return (tv.tv_sec < 0) == (multiplier < 0) ?
    Helper::mul(abs(tv), Helper::abs(multiplier)) :
    -Helper::mul(abs(tv), Helper::abs(multiplier));
}

inline Gears::Time
operator /(const timeval& tv, int divisor) throw ()
{
  return (tv.tv_sec < 0) == (divisor < 0) ?
    Helper::div(abs(tv), Helper::abs(divisor)) :
    -Helper::div(abs(tv), Helper::abs(divisor));
}


namespace Gears
{
  //
  // Time class
  //

  inline void
  Time::set(time_t time_sec, suseconds_t usec) throw ()
  {
    tv_sec = time_sec;
    tv_usec = usec;
  }

  inline
  Time::Time(const Gears::SubString& value, const char* format,
    bool strict)
    throw (InvalidArgument, Exception, Gears::Exception)
  {
    set(value, format, strict);
  }

  inline void
  Time::print(const timeval& time, std::ostream& ostr)
    throw (Gears::Exception)
  {
    ostr << Time(time);
  }

  inline int
  Time::compare(const tm& t1, const tm& t2) throw ()
  {
    int diff = t1.tm_year - t2.tm_year;

    if(diff == 0)
    {
      diff = t1.tm_mon - t2.tm_mon;
    }

    if(diff == 0)
    {
      diff = t1.tm_mday - t2.tm_mday;
    }

    if(diff == 0)
    {
      diff = t1.tm_hour - t2.tm_hour;
    }

    if(diff == 0)
    {
      diff = t1.tm_min - t2.tm_min;
    }

    if(diff == 0)
    {
      diff = t1.tm_sec - t2.tm_sec;
    }

    return diff;
  }

  inline unsigned
  Time::month(const Gears::SubString& mon)
    throw (InvalidArgument, Exception)
  {
    static const char* FUN = "Time::month()";

    if(mon.empty())
    {
      ErrorStream ostr;
      ostr << FUN << ": empty month specified";
      throw InvalidArgument(ostr.str());
    }

    for (int i = 0; i < 12; i++)
    {
      if(mon == ExtendedTime::MONTHS_[i])
      {
        return i;
      }
    }

    {
      ErrorStream ostr;
      ostr << FUN << ": invalid month specified '" << mon << "'";
      throw InvalidArgument(ostr.str());
    }
  }

  inline const char*
  Time::month(unsigned month) throw (InvalidArgument, Exception)
  {
    static const char* FUN = "Time::month()";

    if(month > 11)
    {
      ErrorStream ostr;
      ostr << FUN << ": invalid month specified '" << month << "'";
      throw InvalidArgument(ostr.str());
    }

    return ExtendedTime::MONTHS_[month].str.data();
  }

  inline unsigned
  Time::week_day(const Gears::SubString& day)
    throw (InvalidArgument, Exception)
  {
    static const char* FUN = "Time::week_day()";

    if(day.empty())
    {
      ErrorStream ostr;
      ostr << FUN << ": empty day specified";
      throw InvalidArgument(ostr.str());
    }

    for (int i = 0; i < 7; i++)
    {
      if(day == ExtendedTime::DAYS_[i] ||
        day == ExtendedTime::DAYS_FULL_[i])
      {
        return i;
      }
    }

    {
      ErrorStream ostr;
      ostr << FUN << ": invalid day specified '" << day << "'";
      throw InvalidArgument(ostr.str());
    }
  }

  inline const char*
  Time::week_day(unsigned day) throw (InvalidArgument, Exception)
  {
    static const char* FUN = "Time::week_day()";

    if(day > 6)
    {
      ErrorStream ostr;
      ostr << FUN << ": invalid day specified '" << day << "'";
      throw InvalidArgument(ostr.str());
    }

    return ExtendedTime::DAYS_[day].str.data();
  }

  inline ExtendedTime
  Time::get_time(TimeZone tz) const
    throw (Exception, Gears::Exception)
  {
    return ExtendedTime(tv_sec, tv_usec, tz);
  }

  inline ExtendedTime
  Time::get_gm_time() const throw (Exception, Gears::Exception)
  {
    return ExtendedTime(tv_sec, tv_usec, TZ_GMT);
  }

  inline ExtendedTime
  Time::get_local_time() const throw (Exception, Gears::Exception)
  {
    return ExtendedTime(tv_sec, tv_usec, TZ_LOCAL);
  }

  inline
  Time::Print
  Time::print() const throw ()
  {
    return tv_sec > 0 ?
      Print{1, tv_sec, tv_usec} :
      tv_sec ?
        tv_usec ?
          Print{-1, -tv_sec - 1, USEC_MAX - tv_usec} :
          Print{-1, -tv_sec, 0} :
        tv_usec ? Print{1, 0, tv_usec} : Print{0, 0, 0};
  }

  inline
  void
  Time::invert_sign() throw ()
  {
    *this = -*this;
  }

  inline
  long long
  Time::microseconds() const throw ()
  {
    return tv_sec * static_cast<long long>(USEC_MAX) + tv_usec;
  }

  inline
  double
  Time::as_double() const throw ()
  {
    return tv_sec + tv_usec / static_cast<double>(USEC_MAX);
  }

  inline
  void
  Time::set(const Gears::SubString& value, const char* format, bool strict)
    throw (InvalidArgument, Exception, Gears::Exception)
  {
    static const char* FUN = "Time::set()";

    if(format == 0)
    {
      ErrorStream ostr;
      ostr << FUN << ": format is NULL.";
      throw InvalidArgument(ostr.str());
    }

    ExtendedTime time(0, 0, Time::TZ_GMT);

    if(const char* error = time.from_str_(value, format, strict))
    {
      ErrorStream ostr;
      ostr << FUN << ": can't parse string '" << value <<
        "' according to format '" << format << "': " << error;
      throw Exception(ostr.str());
    }
    *this = time;
  }

  inline
  void
  Time::pack(void* buffer) const throw ()
  {
    int32_t* buf = static_cast<int32_t*>(buffer);
    buf[0] = static_cast<int32_t>(tv_sec);
    buf[1] = static_cast<int32_t>(tv_usec);
  }

  inline
  void
  Time::unpack(const void* buffer) throw ()
  {
    const int32_t* buf = static_cast<const int32_t*>(buffer);
    set(static_cast<time_t>(buf[0]), static_cast<suseconds_t>(buf[1]));
  }

  inline
  Time&
  Time::operator +=(const Time& time) throw ()
  {
    return *this = *this + time;
  }

  inline
  Time&
  Time::operator +=(time_t time) throw ()
  {
    tv_sec += time;
    return *this;
  }

  inline
  Time&
  Time::operator -=(const Time& time) throw ()
  {
    return *this = *this - time;
  }

  inline
  Time&
  Time::operator -=(time_t time) throw ()
  {
    tv_sec -= time;
    return *this;
  }

  inline
  Time&
  Time::operator *=(int multiplier) throw ()
  {
    return *this = *this * multiplier;
  }

  inline
  Time&
  Time::operator /=(int divisor) throw ()
  {
    return *this = *this / divisor;
  }

  inline
  std::string
  Time::gm_f() const throw (Gears::Exception)
  {
    return get_gm_time().format("%F");
  }

  inline
  std::string
  Time::gm_ft() const throw (Gears::Exception)
  {
    return get_gm_time().format("%F %T");
  }

  inline
  void
  Time::set_f(const Gears::SubString& value, bool strict)
    throw (InvalidArgument, Exception, Gears::Exception)
  {
    set(value, "%Y-%m-%d", strict);
  }

  inline
  void
  Time::set_ft(const Gears::SubString& value, bool strict)
    throw (InvalidArgument, Exception, Gears::Exception)
  {
    set(value, "%Y-%m-%d %H:%M:%S", strict);
  }

  template <typename Hash>
  void
  hash_add(Hash& hash, const Time& key) throw ()
  {
    union
    {
      uint64_t value;
      uint32_t v32[2];
    } v = { (static_cast<uint64_t>(key.tv_sec) << 24) |
      static_cast<uint64_t>(key.tv_usec) };
    hash.add(&v.value, sizeof(v.value));
  }


  //
  // ClockTimeOfDay class
  //

  inline
  Time
  ClockTimeOfDay::operator ()() const throw ()
  {
    return Time::get_time_of_day();
  }

  //
  // Timer class
  //

  template <typename TimeStamp, typename Clock>
  GeneralTimer<TimeStamp, Clock>::GeneralTimer() throw ()
    : started_(false)
  {
  }

  template <typename TimeStamp, typename Clock>
  void
  GeneralTimer<TimeStamp, Clock>::start() throw ()
  {
    started_ = true;
    start_ = clock_();
  }

  template <typename TimeStamp, typename Clock>
  void
  GeneralTimer<TimeStamp, Clock>::stop() throw ()
  {
    Time stop = clock_();
    if(started_)
    {
      stop_ = stop;
      started_ = false;
    }
  }

  template <typename TimeStamp, typename Clock>
  TimeStamp
  GeneralTimer<TimeStamp, Clock>::start_time() const throw ()
  {
    return start_;
  }

  template <typename TimeStamp, typename Clock>
  TimeStamp
  GeneralTimer<TimeStamp, Clock>::stop_time() const throw ()
  {
    return stop_;
  }

  template <typename TimeStamp, typename Clock>
  TimeStamp
  GeneralTimer<TimeStamp, Clock>::elapsed_time() const throw ()
  {
    return stop_ - start_;
  }

  template <typename TimeStamp, typename Clock>
  void
  GeneralTimer<TimeStamp, Clock>::stop_set(TimeStamp& timeout) throw ()
  {
    stop();
    timeout = elapsed_time();
  }

  template <typename TimeStamp, typename Clock>
  void
  GeneralTimer<TimeStamp, Clock>::stop_add(TimeStamp& timeout) throw ()
  {
    stop();
    timeout += elapsed_time();
  }

  //
  // TimeMeter classes
  //

  template <typename Timer, const bool ADDITIVE>
  TimeMeter<Timer, ADDITIVE>::TimeMeter(Time& time) throw ()
    : time_(time)
  {
    Timer::start();
  }

  template <typename Timer, const bool ADDITIVE>
  TimeMeter<Timer, ADDITIVE>::~TimeMeter() throw ()
  {
    if(ADDITIVE)
    {
      Timer::stop_add(time_);
    }
    else
    {
      Timer::stop_set(time_);
    }
  }
}


//
// Global functions
//

// Comparison functions

inline
bool
operator ==(const timeval& tv1, const timeval& tv2) throw ()
{
  return tv1.tv_sec == tv2.tv_sec && tv1.tv_usec == tv2.tv_usec;
}

inline
bool
operator !=(const timeval& tv1, const timeval& tv2) throw ()
{
  return !(tv1 == tv2);
}

inline
bool
operator <(const timeval& tv1, const timeval& tv2) throw ()
{
  return tv1.tv_sec < tv2.tv_sec ||
    (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec < tv2.tv_usec);
}

inline
bool
operator >(const timeval& tv1, const timeval& tv2) throw ()
{
  return tv2 < tv1;
}

inline
bool
operator <=(const timeval& tv1, const timeval& tv2) throw ()
{
  return !(tv2 < tv1);
}

inline
bool
operator >=(const timeval& tv1, const timeval& tv2) throw ()
{
  return !(tv1 < tv2);
}

#endif
