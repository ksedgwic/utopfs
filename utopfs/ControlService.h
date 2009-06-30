#ifndef ControlService_h__
#define ControlService_h__

/// @file ControlService.h
/// Utopia FileSystem ControlService Object.
///
/// ControlService for Utopfs FileSystem.

#include <string>

#include <ace/Svc_Handler.h>
#include <ace/LSOCK_Stream.h>

#include "utpfwd.h"

typedef ACE_Svc_Handler<ACE_LSOCK_STREAM, ACE_NULL_SYNCH> ACE_LSOCK_Service;

class ControlService : public ACE_LSOCK_Service
{
public:
    ControlService(utp::BlockStoreHandle const & i_bsh,
                   utp::FileSystemHandle const & i_fsh)
        : m_bsh(i_bsh)
        , m_fsh(i_fsh)
    {}

    virtual int handle_input(ACE_HANDLE i_fd = ACE_INVALID_HANDLE);

    virtual int handle_output(ACE_HANDLE i_fd = ACE_INVALID_HANDLE);

    virtual int handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask);

    int open(void * = 0);

private:
    utp::BlockStoreHandle		m_bsh;
    utp::FileSystemHandle		m_fsh;
};

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_ControlService_h__
