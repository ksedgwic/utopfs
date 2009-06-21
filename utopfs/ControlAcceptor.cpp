#include <iostream>

#include <ace/Reactor.h>

#include "Except.h"
#include "FileSystem.h"

#include "fuselog.h"

#include "ControlAcceptor.h"
#include "ControlService.h"

using namespace std;
using namespace utp;

ControlAcceptor::ControlAcceptor(FileSystemHandle const & i_fsh,
                                 string const & i_sockpath)
    : m_fsh(i_fsh)
    , m_sockpath(i_sockpath)
    , m_reactor(ACE_Reactor::instance())
{
    LOG(lgr, 4, "CTOR");
}

ControlAcceptor::~ControlAcceptor()
{
    LOG(lgr, 4, "DTOR");
}

int
ControlAcceptor::handle_input(ACE_HANDLE i_fd)
{
    LOG(lgr, 4, "handle_input");

    auto_ptr<ControlService> cntrlsvc(new ControlService(m_fsh));

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
ControlAcceptor::handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask)
{
    LOG(lgr, 4, "handle_close");

    if (m_acceptor.get_handle() != ACE_INVALID_HANDLE)
    {
        ACE_Reactor_Mask msk =
            ACE_Event_Handler::ACCEPT_MASK |
            ACE_Event_Handler::DONT_CALL;

        m_reactor->remove_handler(this, msk);
        m_acceptor.close();
    }

    return 0;
}

int
ControlAcceptor::handle_timeout(ACE_Time_Value const & current_time,
                           void const * act)
{
    LOG(lgr, 4, "handle_timeout");

    // Later we'll do more stuff, for now we only open ...
    open();

    return 0;
}

void
ControlAcceptor::init()
{
    LOG(lgr, 4, "init");

    // Schedule a timeout to get the socket open on a different thread.
    //
    // BOGUS - we need a couple of seconds delay here or we deadlock.
    // Surely there is a better way?  Can we trap some other FUSE
    // event and init then?
    //
    m_reactor->schedule_timer(this, NULL, ACE_Time_Value(2, 0));
}

void
ControlAcceptor::open()
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

    m_reactor->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
    
    LOG(lgr, 4, "control socket open");
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
