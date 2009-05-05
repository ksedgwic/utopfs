#ifndef utp_T64_h__
#define utp_T64_h__

#include <iosfwd>

#include <ace/Basic_Types.h>
#include <ace/Time_Value.h>

#if defined(LINUX)
#include <sys/time.h>
#include <limits.h>
#elif defined(WIN32)
#include <time.h>
#include <sys/timeb.h>
#endif

#include "Types.h"
#include "utpexp.h"

namespace utp {

class UTP_EXP T64
{
public:
    // ----------------------------------------------------------------
    // Factories
    // ----------------------------------------------------------------

    /// Returns the current time.
    ///
    static inline T64 now()
    {
#if defined(LINUX)
        struct timeval tv;
        gettimeofday(&tv, 0);
        return T64(tv);
#elif defined(WIN32)
        struct timeb tb;
        ftime(&tb);
        return T64(tb);
#else
#error "Unknown architecture"
#endif
    }

    /// Creates a T64 from uSec as a int64
    static inline T64 usec(int64 t) { return T64(t); }

    /// Creates a T64 from seconds value as double.
    static inline T64 sec(double s)
    {
        return T64::usec(int64(s * 1.0e6));
    }

    // ----------------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------------

    /// Default constructor.
    inline T64() : m_usec(0) {}

    /// Constructor from uSec as int64
    inline T64(int64 t) : m_usec(t) {}

    explicit T64(struct tm const * tmptr)
        : m_usec(int64(mktime(const_cast<struct tm *>(tmptr))) * 1000000LL) {}

#if defined(LINUX)
    /// Constructor from struct timeval.
    inline T64(struct timeval const & tv)
        : m_usec(((int64(tv.tv_sec)) * 1000000LL) + tv.tv_usec) {}
#elif defined(WIN32)
    /// Constructor from struct timeb.
    inline T64(struct timeb const & tb)
        : m_usec(((int64(tb.time)) * 1000000LL) + (tb.millitm * 1000LL)) {}
#else
#error "Unknown architecture"
#endif

    inline T64(ACE_Time_Value const & i_tv)
        : m_usec(i_tv.sec() * 1000000LL + i_tv.usec()) {}

    // ----------------------------------------------------------------
    // Conversions
    // ----------------------------------------------------------------

    /// Returns mSec as a long
    /// Returns LONG_MAX if the result is bigger then fits in a LONG ...
    inline long msec() const
    {
		int64 val = m_usec / 1000LL;
		return
            val < ACE_INT32_MIN ? ACE_INT32_MIN :
            val > ACE_INT32_MAX ? ACE_INT32_MAX :
            int32(val);
    };

    /// Returns uSec as a 64 bit integer
    inline int64 usec() const
    {
        return m_usec;
    }

    /// Returns seconds as a double
    inline double sec() const
    {
        return double(m_usec) / 1.0e6;
    }

    /// Cast to ACE_Time_Value.
    inline operator ACE_Time_Value() const
    {
        return ACE_Time_Value(long(m_usec / 1000000LL),
							  long(m_usec % 1000000LL));
    }

    // ----------------------------------------------------------------
    // Operators
    // ----------------------------------------------------------------

    /// Addition operator.
    inline T64 operator+(T64 t) const
    {
        return T64(m_usec + t.m_usec);
    }

    /// Subtraction operator.
    inline T64 operator-(T64 t) const
    {
        return T64(m_usec - t.m_usec);
    }

    /// Sum operator.
    inline T64 & operator+=(T64 t)
    {
        m_usec += t.m_usec;
        return *this;
    }

    /// Negative sum operator.
    inline T64 & operator-=(T64 t)
    {
        m_usec -= t.m_usec;
        return *this;
    }

    /// Less-than predicate.
    inline bool operator<(T64 const & b) const
    {
        return m_usec < b.m_usec;
    }

    /// Less-than-or-equal predicate.
    inline bool operator<=(T64 const & b) const
    {
        return m_usec <= b.m_usec;
    }

    /// Greater-than predicate.
    inline bool operator>(T64 const & b) const
    {
        return m_usec > b.m_usec;
    }

    /// Greater-than-or-equal predicate.
    inline bool operator>=(T64 const & b) const
    {
        return m_usec >= b.m_usec;
    }

    /// Equality operator.
    inline bool operator==(T64 const & b) const
    {
        return m_usec == b.m_usec;
    }

    /// Non-equality operator.
    inline bool operator!=(T64 const & b) const
    {
        return m_usec != b.m_usec;
    }

private:
    int64				m_usec;
};

/// Inserts a T64 in human-readable format into output stream.
UTP_EXP std::ostream &
operator<<(std::ostream & ostrm, T64 const & t);

/// Extracts a T64 from input stream.
UTP_EXP std::istream &
operator>>(std::istream & istrm, T64 & t);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_T64_h__
