#ifndef StatsLogger_h__
#define StatsLogger_h__

/// @file StatsLogger.h
/// Utopia FileSystem StatsLogger Object.
///
/// StatsLogger for Utopfs FileSystem.

#include <string>

#include <ace/Event_Handler.h>
#include <ace/LSOCK_Acceptor.h>

#include "Assembly.h"
#include "utpfwd.h"

class StatsLogger : public ACE_Event_Handler
{
public:
    StatsLogger(utp::Assembly * i_ap,
                std::string const & i_statspath,
                double i_statssecs);

    virtual ~StatsLogger();

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    void init();

    void term();

    void do_stats() const;

    void format_stats(std::ostream & i_ostrm,
                      std::string const & i_prefix,
                      utp::StatSet const & i_ss) const;

private:
    utp::Assembly *			m_ap;
    utp::BlockStoreHandle	m_bsh;
    utp::FileSystemHandle	m_fsh;
    std::string				m_statspath;
    double					m_statssecs;
    ACE_Reactor *			m_reactor;
};

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_StatsLogger_h__
