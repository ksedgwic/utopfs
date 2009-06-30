#include <iostream>

#include <ace/Reactor.h>

#include "Except.h"
#include "BlockStore.h"
#include "FileSystem.h"

#include "fuselog.h"
#include "ControlService.h"

using namespace std;
using namespace utp;

int
ControlService::handle_input(ACE_HANDLE i_fd)
{
    LOG(lgr, 4, "handle_input");

    char buffer[8192];

    ssize_t rv = peer().recv(buffer, sizeof(buffer));
    if (rv <= 0)
    {
        LOG(lgr, 4, "connection closed");
        return -1;
    }

    ostringstream retstrm;
    string cmdstr(buffer, rv);
    if (cmdstr == "refresh")
    {
        size_t nblocks = m_fsh->fs_refresh();
        retstrm << "refreshed " << nblocks << " blocks";
        
    }
    else
    {
        retstrm << "unrecognized control command: \"" << cmdstr << "\"";
        LOG(lgr, 2, retstrm.str());
    }

    string const & retstr = retstrm.str();
    rv = peer().send(retstr.data(), retstr.size());

    // All done.
    return -1;
}

int
ControlService::handle_output(ACE_HANDLE i_fd)
{
    LOG(lgr, 4, "handle_output");

    return -1;
}

int
ControlService::handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask)
{
    LOG(lgr, 4, "handle_close");

    if (i_mask == ACE_Event_Handler::WRITE_MASK)
        return 0;

    return ACE_LSOCK_Service::handle_close(i_handle, i_mask);
}

int
ControlService::open(void * ptr)
{
    LOG(lgr, 4, "open");

    if (ACE_LSOCK_Service::open(ptr) == -1)
        return -1;

    return 0;
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
