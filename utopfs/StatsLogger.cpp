#include <fstream>
#include <iostream>
#include <stdexcept>

#include <ace/Reactor.h>

#include "Stats.pb.h"

#include "Except.h"
#include "BlockStore.h"
#include "FileSystem.h"

#include "fuselog.h"

#include "StatsLogger.h"

using namespace std;
using namespace utp;

StatsLogger::StatsLogger(Assembly * i_ap,
                         string const & i_statspath,
                         double i_statssecs)
    : m_ap(i_ap)
    , m_bsh(i_ap->bsh())
    , m_fsh(i_ap->fsh())
    , m_statspath(i_statspath)
    , m_statssecs(i_statssecs)
    , m_reactor(ACE_Reactor::instance())
{
    LOG(lgr, 6, "CTOR");
}

StatsLogger::~StatsLogger()
{
    LOG(lgr, 6, "DTOR");
}

int
StatsLogger::handle_timeout(ACE_Time_Value const & current_time,
                           void const * act)
{
    LOG(lgr, 6, "handle_timeout");

    do_stats();

    return 0;
}

void
StatsLogger::init()
{
    LOG(lgr, 4, "init, logging every " << m_statssecs << " seconds");

    // BOGUS - we need a couple of seconds delay here or we deadlock.
    // Surely there is a better way?  Can we trap some other FUSE
    // event and init then?
    //
    ACE_Time_Value initdelay(2, 0);

    // Register the stats timeouts.
    {
        ACE_Time_Value period;
        period.set(m_statssecs);
        m_reactor->schedule_timer(this, NULL, initdelay, period);
    }
}

void
StatsLogger::term()
{
    m_reactor->cancel_timer(this);
}

void
StatsLogger::do_stats() const
{
    try
    {
        StatSet ss;
        m_ap->get_stats(ss);

        // Open the stats log.
        ofstream ostrm(m_statspath.c_str(), ios_base::app);
        ostrm << T64::now();
        format_stats(ostrm, "", ss);
        ostrm << endl;
    }
    catch (std::exception const & ex)
    {
        LOG(lgr, 1, "exception in do_stats: " << ex.what());
    }
}

void
StatsLogger::format_stats(ostream & i_ostrm,
                         string const & i_prefix,
                         StatSet const & i_ss) const
{
    int64 t0 = T64::now().usec();

    string pfx =
        i_prefix.empty() ? i_ss.name() : i_prefix + '.' + i_ss.name();

    if (i_ss.rec_size() > 0)
    {
        i_ostrm << ' ' << pfx << '[';

        bool isfirst = true;

        for (int ii = 0; ii < i_ss.rec_size(); ++ii)
        {
            StatRec const & sr = i_ss.rec(ii);
            int64 const & val = sr.value();
            for (int jj = 0; jj < sr.format_size(); ++jj)
            {
                char buffer[256];
                StatFormat const & sf = sr.format(jj);
                double factor = sf.has_factor() ? sf.factor() : 1.0;

                string namestr = pfx + '.' + sr.name();
                double wval;
                double tval;

                switch (sf.fmttype())
                {
                case SF_VALUE:
                    wval = double(val) * factor;
                    break;

                case SF_DELTA:
                    tval = double(t0 - m_t0[namestr]) / 1e6;
                    wval = tval > 0.0
                        ? double(val - m_val[namestr]) * factor / tval
                        : 0.0;
                    
                    m_val[namestr] = val;
                    m_t0[namestr] = t0;
                    break;
                }

                if (wval != 0.0)
                {
                    snprintf(buffer, sizeof(buffer), sf.fmtstr().c_str(), wval);

                    // Emit a spacer for all but the first.
                    if (isfirst)
                        isfirst = false;
                    else
                        i_ostrm << ' ';

                    i_ostrm << sr.name() << '=' << buffer;
                }
            }
        }

        i_ostrm << ']';
    }

    for (int ii = 0; ii < i_ss.subset_size(); ++ii)
    {
        StatSet const & ss = i_ss.subset(ii);
        format_stats(i_ostrm, pfx, ss);
    }
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
