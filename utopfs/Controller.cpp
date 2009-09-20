#include <fstream>
#include <iostream>
#include <stdexcept>

#include <ace/Reactor.h>

#include "Stats.pb.h"

#include "Except.h"
#include "BlockStore.h"
#include "FileSystem.h"

#include "fuselog.h"

#include "Controller.h"
#include "ControlService.h"

using namespace std;
using namespace utp;

Controller::Controller(Assembly * i_ap,
                       string const & i_controlpath,
                       string const & i_statspath,
                       double i_statssecs,
                       double i_syncsecs)
    : m_opened(false)
    , m_ap(i_ap)
    , m_bsh(i_ap->bsh())
    , m_fsh(i_ap->fsh())
    , m_controlpath(i_controlpath)
    , m_statspath(i_statspath)
    , m_statssecs(i_statssecs)
    , m_syncsecs(i_syncsecs)
    , m_reactor(ACE_Reactor::instance())
{
    LOG(lgr, 4, "CTOR");

    ostringstream pathstrm;
    pathstrm << "/var/tmp/utopfs_control." << getpid();
    m_sockpath = pathstrm.str();
}

Controller::~Controller()
{
    LOG(lgr, 4, "DTOR");
}

int
Controller::handle_input(ACE_HANDLE i_fd)
{
    LOG(lgr, 4, "handle_input");

    auto_ptr<ControlService> cntrlsvc(new ControlService(m_bsh, m_fsh));

    if (m_acceptor.accept(cntrlsvc->peer()) == -1)
    {
        LOG(lgr, 2, "failed to accept client connection: "
            << ACE_OS::strerror(errno));
        return -1;
    }

    cntrlsvc->reactor(m_reactor);

    if (cntrlsvc->open() == -1)
        cntrlsvc->handle_close(ACE_INVALID_HANDLE, 0);

    cntrlsvc.release();

    return 0;
}

int
Controller::handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask)
{
    LOG(lgr, 4, "handle_close");

    // Cancel any timers.
    m_reactor->cancel_timer(this);

    if (m_acceptor.get_handle() != ACE_INVALID_HANDLE)
    {
        m_reactor->remove_handler(this, ACCEPT_MASK | DONT_CALL);
        m_acceptor.close();
    }

    return 0;
}

int
Controller::handle_timeout(ACE_Time_Value const & current_time,
                           void const * act)
{
    long which = (long) act;

    LOG(lgr, 4, "handle_timeout " << which);

    switch (which)
    {
    case 0:
        // Sync timeout
        // The first time through we open our control socket.  Subsequent
        // calls perform periodic duties.
        if (!m_opened)
            do_open();
        else
            do_sync();
        break;

    case 1:
        // Stats timeout
        do_stats();
        break;
    }

    return 0;
}

void
Controller::init()
{
    LOG(lgr, 4, "init, syncing every " << m_syncsecs << " seconds");

    // Schedule a timeout to get the socket open on a different thread.
    //
    // BOGUS - we need a couple of seconds delay here or we deadlock.
    // Surely there is a better way?  Can we trap some other FUSE
    // event and init then?
    //
    ACE_Time_Value initdelay(2, 0);

    // Register the sync timeouts.
    {
        ACE_Time_Value period;
        period.set(m_syncsecs);
        m_reactor->schedule_timer(this, (void *) 0, initdelay, period);
    }

    // Register the stats timeouts.
    {
        ACE_Time_Value period;
        period.set(m_statssecs);
        m_reactor->schedule_timer(this, (void *) 1, initdelay, period);
    }
}

void
Controller::term()
{
    m_reactor->cancel_timer(this, DONT_CALL);

    // Remove the control socket.
    unlink(m_sockpath.c_str());
}

void
Controller::do_open()
{
    LOG(lgr, 4, "open");

    ACE_UNIX_Addr server_addr(m_sockpath.c_str());
    
    if (m_acceptor.open(server_addr) == -1)
        throwstream(InternalError, FILELINE
                    << "open of control socket failed: "
                    << ACE_OS::strerror(errno));

    else if (m_acceptor.get_local_addr(server_addr) == -1)
        throwstream(InternalError, FILELINE
                    << "get local addr of control socket failed: "
                    << ACE_OS::strerror(errno));

    m_reactor->register_handler(this, ACCEPT_MASK);
    
    LOG(lgr, 4, "control socket " << m_sockpath << " open");

    // Create symbolic link in the .utopfs directory.
    if (symlink(m_sockpath.c_str(), m_controlpath.c_str()))
        throwstream(InternalError, FILELINE
                    << "symlink failed: " << ACE_OS::strerror(errno));

    m_opened = true;
}

void
Controller::do_sync()
{
    try
    {
        // Synchronize the filesystem to the blockstore.
        m_fsh->fs_sync();

        // Persist the blockstore.
        m_bsh->bs_sync();
    }
    catch (std::exception const & ex)
    {
        LOG(lgr, 1, "exception in do_sync: " << ex.what());
    }
}

void
Controller::do_stats() const
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
Controller::format_stats(ostream & i_ostrm,
                         string const & i_prefix,
                         StatSet const & i_ss) const
{
    string pfx =
        i_prefix.empty() ? i_ss.name() : i_prefix + '.' + i_ss.name();

    for (int ii = 0; ii < i_ss.rec_size(); ++ii)
    {
        StatRec const & sr = i_ss.rec(ii);
        int64 const & val = sr.value();
        for (int jj = 0; jj < sr.format_size(); ++jj)
        {
            char buffer[256];
            StatFormat const & sf = sr.format(jj);
            double factor = sf.has_factor() ? sf.factor() : 1.0;

            double wval;

            switch (sf.fmttype())
            {
            case SF_VALUE:
                wval = double(val) * factor;
                break;

            case SF_DELTA:
                throwstream(InternalError, FILELINE
                            << "SF_DELTA unimplemented");
                break;
            }

            snprintf(buffer, sizeof(buffer), sf.fmtstr().c_str(), wval);

            i_ostrm << ' ' << pfx << '.' << sr.name() << '=' << buffer;
        }
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
