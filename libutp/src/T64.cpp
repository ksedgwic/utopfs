#if defined(LINUX)
#include <time.h>
#endif

#include <iomanip>
#include <iostream>
#include <sstream>

#include "ace/OS_NS_time.h"

#include "T64.h"

using namespace std;

namespace utp {

ostream &
operator<<(ostream & ostrm, T64 const & t)
{
    T64 const M(1000000LL);
    unsigned secs = static_cast<unsigned>(t.usec() / M.usec());
    unsigned usecs = static_cast<unsigned>(t.usec() % M.usec());
    unsigned msecs = usecs / 1000;

    // Relative or absolute?
    if (secs < 500e6)
    {
        // Small value, relative.
        //
        ostringstream vstrm;
        vstrm << secs << '.' << setw(3) << setfill('0') << msecs;
        ostrm << vstrm.str();
    }
    else
    {
        // Large value, absolute.
        //
        struct tm tim;
        time_t tt(secs);
        ACE_OS::localtime_r(&tt, &tim);
        char datebuf[256];
        strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M:%S", &tim);
        char outbuf[256];
        ACE_OS::snprintf(outbuf, sizeof(outbuf), "%s.%03d", datebuf, msecs);
        ostrm << outbuf;
    }

    return ostrm;
}
istream &
operator>>(istream & istrm, T64 & t)
{
    // Read the value into a string.
    string firstbuf;
    if (!(istrm >> firstbuf))
        return istrm;

    // Is this a formatted date-time?
    struct tm tim;
    int msec;
    int matched = sscanf(firstbuf.c_str(), "%4d-%2d-%2d",
                         &tim.tm_year, &tim.tm_mon, &tim.tm_mday);
    if (matched == 3)
    {
        string secondbuf;
        if (!(istrm >> secondbuf))
            return istrm;

        matched = sscanf(secondbuf.c_str(), "%2d:%2d:%2d.%03d",
                         &tim.tm_hour, &tim.tm_min, &tim.tm_sec, &msec);
        if (matched == 4)
        {
            tim.tm_year -= 1900;
            tim.tm_mon -= 1;
            tim.tm_isdst = -1;
            t = T64(&tim);
            t += T64(msec * 1000);
            return istrm;
        }

        // Failure.
        istrm.setstate(std::ios::failbit);
        return istrm;
    }

    // Is this a floating point value?
    float fsecs;
    matched = sscanf(firstbuf.c_str(), "%f", &fsecs);
    if (matched == 1)
    {
        t = T64::sec(fsecs);
        return istrm;
    }

    // Failure.
    istrm.setstate(std::ios::failbit);
    return istrm;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
