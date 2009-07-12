#include <iostream>
#include <stdexcept>

#include <ace/Reactor.h>

#include "Except.h"
#include "BlockStore.h"
#include "FileSystem.h"

#include "fuselog.h"

#include "Controller.h"
#include "ControlService.h"

using namespace std;
using namespace utp;

Controller::Controller(BlockStoreHandle const & i_bsh,
                       FileSystemHandle const & i_fsh,
                       string const & i_controlpath,
                       double i_syncsecs)
    : m_opened(false)
    , m_bsh(i_bsh)
    , m_fsh(i_fsh)
    , m_controlpath(i_controlpath)
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
    LOG(lgr, 4, "handle_timeout");

    // The first time through we open our control socket.  Subsequent
    // calls perform periodic duties.

    if (!m_opened)
        open();
    else
        periodic();

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
    ACE_Time_Value period;
    period.set(m_syncsecs);
    m_reactor->schedule_timer(this, NULL, initdelay, period);
}

void
Controller::open()
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
Controller::term()
{
    m_reactor->cancel_timer(this, DONT_CALL);

    // Remove the control socket.
    unlink(m_sockpath.c_str());
}

void
Controller::periodic()
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
        LOG(lgr, 1, "exception in periodic: " << ex.what());
    }
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
