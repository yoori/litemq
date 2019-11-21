#include "StringManip.hpp"
#include "Time.hpp"

namespace Gears
{
  const std::size_t Time::TIME_PACK_LEN;

  const unsigned long Time::TIME_LEN;

  const suseconds_t Time::USEC_MAX;

  const Time Time::ZERO;
  const Time Time::ONE_SECOND(1l);
  const Time Time::ONE_MINUTE(60l);
  const Time Time::ONE_HOUR(60l * 60l);
  const Time Time::ONE_DAY(24l * 60l * 60l);
  const Time Time::ONE_WEEK(7l * 24l * 60l * 60l);

  const Ascii::Caseless ExtendedTime::DAYS_[] =
  {
    Ascii::Caseless("Sun"),
    Ascii::Caseless("Mon"),
    Ascii::Caseless("Tue"),
    Ascii::Caseless("Wed"),
    Ascii::Caseless("Thu"),
    Ascii::Caseless("Fri"),
    Ascii::Caseless("Sat")
  };

  const Ascii::Caseless ExtendedTime::DAYS_FULL_[] =
  {
    Ascii::Caseless("Sunday"),
    Ascii::Caseless("Monday"),
    Ascii::Caseless("Tuesday"),
    Ascii::Caseless("Wednesday"),
    Ascii::Caseless("Thursday"),
    Ascii::Caseless("Friday"),
    Ascii::Caseless("Saturday")
  };

  const Ascii::Caseless ExtendedTime::MONTHS_[] =
  {
    Ascii::Caseless("Jan"),
    Ascii::Caseless("Feb"),
    Ascii::Caseless("Mar"),
    Ascii::Caseless("Apr"),
    Ascii::Caseless("May"),
    Ascii::Caseless("Jun"),
    Ascii::Caseless("Jul"),
    Ascii::Caseless("Aug"),
    Ascii::Caseless("Sep"),
    Ascii::Caseless("Oct"),
    Ascii::Caseless("Nov"),
    Ascii::Caseless("Dec")
  };

  const Ascii::Caseless ExtendedTime::MONTHS_FULL_[] =
  {
    Ascii::Caseless("January"),
    Ascii::Caseless("February"),
    Ascii::Caseless("March"),
    Ascii::Caseless("April"),
    Ascii::Caseless("May"),
    Ascii::Caseless("June"),
    Ascii::Caseless("July"),
    Ascii::Caseless("August"),
    Ascii::Caseless("September"),
    Ascii::Caseless("October"),
    Ascii::Caseless("November"),
    Ascii::Caseless("December")
  };

  static const int DAYS[2][12] =
  {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
  };

  time_t
  gm_to_time(const tm& et) throw ()
  {
    const long YEARS = et.tm_year - 70;
    return ((YEARS * 365) + (YEARS + 1) / 4 +
      DAYS[(YEARS & 3) == 2][et.tm_mon] + et.tm_mday - 1) * 86400 +
      et.tm_hour * 3600 + et.tm_min * 60 + et.tm_sec;
  }

  void
  time_to_gm(time_t time, tm& et) throw ()
  {
    memset(&et, 0, sizeof(et));
    et.tm_sec = time % 60;
    time /= 60;
    et.tm_min = time % 60;
    time /= 60;
    et.tm_hour = time % 24;
    time /= 24;
    et.tm_wday = (time + 4) % 7;
    long years = time / (4 * 365 + 1) * 4;
    time %= 4 * 365 + 1;
    long leap = 0;
    if (time >= 365)
    {
      if (time >= 365 * 2)
      {
        if (time >= 365 * 3 + 1)
        {
          years += 3;
          time -= 365 * 3 + 1;
        }
        else
        {
          years += 2;
          time -= 365 * 2;
          leap = 1;
        }
      }
      else
      {
        years++;
        time -= 365;
      }
    }
    et.tm_year = years + 70;
    et.tm_yday = time;
    const int* const CDAYS(DAYS[leap]);
    const int* const MONTH(
      std::lower_bound(CDAYS + 1, CDAYS + 12, ++time) - 1);
    et.tm_mon = MONTH - CDAYS;
    et.tm_mday = time - *MONTH;
  }

  namespace
  {
    bool
    check_name(const char*& src, size_t& size,
      const Ascii::Caseless& cl) throw ()
    {
      if (cl.start(SubString(src, size)))
      {
        src += cl.str.size();
        size -= cl.str.size();
        return true;
      }
      return false;
    }

    bool
    check_names(const char*& src, size_t& size,
      const Ascii::Caseless& cl1,
      const Ascii::Caseless& cl2) throw ()
    {
      return check_name(src, size, cl1) || check_name(src, size, cl2);
    }

    template <const size_t SIZE, typename T>
    bool
    read_number(const char*& src, size_t& size, T& number, bool strict)
      throw ()
    {
      if (!size || !Ascii::NUMBER(*src) ||
        (strict && size < SIZE))
      {
        return false;
      }

      size_t left = std::min(SIZE, size);
      number = 0;
      do
      {
        number = number * 10 + (*src - '0');
        src++;
        size--;
      }
      while (--left && Ascii::NUMBER(*src));
      return !strict || !left;
    }

    bool
    add_str(char*& str, size_t& size, size_t length,
      const SubString& src) throw ()
    {
      if (size + src.size() > length)
      {
        return false;
      }
      Gears::CharTraits<char>::copy(str, src.data(), src.size());
      str += src.size();
      size += src.size();
      return true;
    }

    template <typename T>
    bool
    add_num(char*& str, size_t& size, size_t length, T number) throw ()
    {
      char buf[64];
      size_t ns = StringManip::int_to_str(number, buf, sizeof(buf));
      return ns && add_str(str, size, length, SubString(buf, ns));
    }

    template <const size_t SIZE, typename T>
    bool
    add_num(char*& str, size_t& size, size_t length, T number) throw ()
    {
      char buf[SIZE];
      size_t i = SIZE;
      T d = 1;
      do
      {
        i--;
        d *= 10;
        buf[i] = number % 10 + '0';
        number /= 10;
      }
      while (i);
      return add_str(str, size, length, SubString(buf, SIZE));
    }
  }


  //
  // ExtendedTime class
  //

  ExtendedTime::ExtendedTime(time_t sec, suseconds_t usec, Time::TimeZone tz)
    throw (Exception, Gears::Exception)
  {
    static const char* FUN = "ExtendedTime::ExtendedTime()";

    switch (tz)
    {
    case Time::TZ_GMT:
      time_to_gm(sec, *this);
      break;

    case Time::TZ_LOCAL:
      if (!localtime_r(&sec, this))
      {
        ErrorStream ostr;
        ostr << FUN << ": localtime_r(" << sec << ") failed";
        throw Exception(ostr.str());
      }
      break;

    default:
      {
        ErrorStream ostr;
        ostr << FUN << ": invalid TZ type";
        throw Exception(ostr.str());
      }
    }

    tm_usec = usec;
    timezone = tz;
  }

  const char*
  ExtendedTime::from_str_(
    const SubString& value,
    const char* format,
    bool strict) throw ()
  {
    const char* v_str = value.data();
    size_t v_size = value.size();

    for (; *format; format++)
    {
      if (*format == '%')
      {
        switch (*++format)
        {
        case '%':
          {
            if (v_size < 1 || *v_str != '%')
            {
              return "% sign is expected but not found";
            }
            v_str++;
            v_size--;
            break;
          }

        case 'a':
        case 'A':
          {
            int wd = 0;
            for (; wd < 7; wd++)
            {
              if (check_names(v_str, v_size, DAYS_FULL_[wd], DAYS_[wd]))
              {
                break;
              }
            }
            if (wd == 7)
            {
              return "weekday name is expected but not found";
            }
            break;
          }

        case 'b':
        case 'B':
        case 'h':
          {
            int m = 0;
            for (; m < 12; m++)
            {
              if (check_names(v_str, v_size, MONTHS_FULL_[m], MONTHS_[m]))
              {
                break;
              }
            }
            if (m == 12)
            {
              return "month name is expected but not found";
            }
            tm_mon = m;
            break;
          }

        case 'd':
        case 'e':
          {
            if (!read_number<2>(v_str, v_size, tm_mday, strict) ||
              tm_mday < 1 || tm_mday > 31)
            {
              return "day of month expected but not found";
            }
            break;
          }

        case 'H':
          {
            if (!read_number<2>(v_str, v_size, tm_hour, strict) ||
              tm_hour > 23)
            {
              return "hours expected but not found";
            }
            break;
          }

        case 'm':
          {
            if (!read_number<2>(v_str, v_size, tm_mon, strict) ||
              tm_mon < 1 || tm_mon > 12)
            {
              return "month number expected but not found";
            }
            tm_mon--;
            break;
          }

        case 'M':
          {
            if (!read_number<2>(v_str, v_size, tm_min, strict) ||
              tm_min > 59)
            {
              return "minutes expected but not found";
            }
            break;
          }

        case 'q':
          {
            if (!read_number<6>(v_str, v_size, tm_usec, strict) ||
              tm_usec >= Time::USEC_MAX)
            {
              return "microseconds expected but not found";
            }
            break;
          }

        case 'S':
          {
            if (!read_number<2>(v_str, v_size, tm_sec, strict) ||
              tm_sec > 59)
            {
              return "seconds expected but not found";
            }
            break;
          }

        case 'Y':
          {
            if (!read_number<4>(v_str, v_size, tm_year, strict) ||
              tm_year < 1970)
            {
              return "year expected but not found";
            }
            tm_year -= 1900;
            break;
          }

        default:
          return "unknown format specifier";
        }
      }
      else
      {
        if (!strict && Ascii::SPACE(*format))
        {
          while (v_size && Ascii::SPACE(*v_str))
          {
            v_str++;
            v_size--;
          };
        }
        else
        {
          if (v_size < 1 || *v_str != *format)
          {
            return "character is expected but not found";
          }
          v_str++;
          v_size--;
        }
      }
    }

    return 0;
  }

  size_t
  ExtendedTime::to_str_(char* str, size_t length, const char* format) const
    throw ()
  {
    size_t size = 0;
    for (; *format; format++)
    {
      if (*format == '%')
      {
        switch (*++format)
        {
        case '%':
          {
            if (length == size)
            {
              return 0;
            }
            size++;
            *str++ = '%';
            break;
          }

        case 'a':
          {
            if (!add_str(str, size, length, DAYS_[tm_wday].str))
            {
              return 0;
            }
            break;
          }

        case 'A':
          {
            if (!add_str(str, size, length, DAYS_FULL_[tm_wday].str))
            {
              return 0;
            }
            break;
          }

        case 'b':
        case 'h':
          {
            if (!add_str(str, size, length, MONTHS_[tm_mon].str))
            {
              return 0;
            }
            break;
          }

        case 'B':
          {
            if (!add_str(str, size, length, MONTHS_FULL_[tm_mon].str))
            {
              return 0;
            }
            break;
          }

        case 'd':
          {
            if (!add_num<2>(str, size, length, tm_mday))
            {
              return 0;
            }
            break;
          }

        case 'e':
          {
            int t = tm_mday / 10;
            char buf[2] = { t ? static_cast<char>(t + '0') : ' ',
              static_cast<char>(tm_mday % 10 + '0') };
            if (!add_str(str, size, length, SubString(buf, 2)))
            {
              return 0;
            }
            break;
          }

        case 'F':
          {
            size_t res = to_str_(str, length - size, "%Y-%m-%d");
            if (!res)
            {
              return 0;
            }
            size += res;
            str += res;
            break;
          }

        case 'H':
          {
            if (!add_num<2>(str, size, length, tm_hour))
            {
              return 0;
            }
            break;
          }

        case 'k':
          {
            if (!add_num(str, size, length, tm_hour))
            {
              return 0;
            }
            break;
          }

        case 'm':
          {
            if (!add_num<2>(str, size, length, tm_mon + 1))
            {
              return 0;
            }
            break;
          }

        case 'M':
          {
            if (!add_num<2>(str, size, length, tm_min))
            {
              return 0;
            }
            break;
          }

        case 'q':
          {
            if (!add_num<6>(str, size, length, tm_usec))
            {
              return 0;
            }
            break;
          }

        case 's':
          {
            Gears::Time t(*this);
            if (!add_num(str, size, length, t.tv_sec))
            {
              return 0;
            }
            break;
          }

        case 'S':
          {
            if (!add_num<2>(str, size, length, tm_sec))
            {
              return 0;
            }
            break;
          }

        case 'T':
          {
            size_t res = to_str_(str, length - size, "%H:%M:%S");
            if (!res)
            {
              return 0;
            }
            size += res;
            str += res;
            break;
          }

        case 'Y':
          {
            if (!add_num<4>(str, size, length, tm_year + 1900))
            {
              return 0;
            }
            break;
          }

        case 'z':
          {
            if (timezone == Time::TZ_GMT)
            {
              if (!add_str(str, size, length,
                SubString("+0000", 5)))
              {
                return 0;
              }
            }
            else
            {
              Gears::ExtendedTime tmp = *this;
              if (mktime(&tmp) == -1)
              {
                return 0;
              }
              int diff = tmp.tm_gmtoff;
              const char SIGN = diff < 0 ? ((diff = -diff), '-') : '+';
              if (!add_str(str, size, length, SubString(&SIGN, 1)))
              {
                return 0;
              }
              diff /= 60;
              if (!add_num<4>(str, size, length,
                diff / 60 * 100 + diff % 60))
              {
                return 0;
              }
            }
            break;
          }

        default:
          return 0;
        }
      }
      else
      {
        if (length == size)
        {
          return 0;
        }
        size++;
        *str++ = *format;
      }
    }

    return size;
  }

  std::string
  ExtendedTime::format(const char* fmt) const
    throw (InvalidArgument, Exception, Gears::Exception)
  {
    static const char* FUN = "ExtendedTime::format()";

    if(fmt == 0)
    {
      ErrorStream ostr;
      ostr << FUN << ": format argument is NULL";
      throw InvalidArgument(ostr.str());
    }

    char str[256];
    size_t length = to_str_(str, sizeof(str), fmt);
    if(!length)
    {
      ErrorStream ostr;
      ostr << FUN << "can't format time with format '" << fmt << "'";
      throw Exception(ostr.str());
    }

    return std::string(str, length);
  }
}

//
// Global functions
//

std::ostream&
operator <<(std::ostream& ostr, const Gears::Time& time)
  throw (Gears::Exception)
{
  char buf[256];
  const Gears::Time::Print& print = time.print();
  snprintf(buf, sizeof(buf), "%s%lu:%.6ld (sec:usec)",
    print.sign < 0 ? "-" : "",
    static_cast<unsigned long int>(print.integer_part),
    static_cast<long int>(print.fractional_part));
  return ostr << buf;
}

std::ostream&
operator <<(std::ostream& ostr, const Gears::ExtendedTime& time)
  throw (Gears::Exception)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "%04u-%02u-%02u.%02u:%02u:%02u.%06u",
    static_cast<unsigned>(time.tm_year + 1900),
    static_cast<unsigned>(time.tm_mon + 1),
    static_cast<unsigned>(time.tm_mday),
    static_cast<unsigned>(time.tm_hour),
    static_cast<unsigned>(time.tm_min),
    static_cast<unsigned>(time.tm_sec),
    static_cast<unsigned>(time.tm_usec));

  ostr.write(buf, 26);

  return ostr;
}
