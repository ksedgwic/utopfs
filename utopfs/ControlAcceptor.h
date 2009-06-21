#ifndef ControlAcceptor_h__
#define ControlAcceptor_h__

/// @file ControlAcceptor.h
/// Utopia FileSystem ControlAcceptor Object.
///
/// ControlAcceptor for Utopfs FileSystem.

#include <string>

#include <ace/Event_Handler.h>
#include <ace/LSOCK_Acceptor.h>

#include "utpfwd.h"

class ControlAcceptor : public ACE_Event_Handler
{
public:
    ControlAcceptor(utp::FileSystemHandle const & i_fsh,
                    std::string const & i_sockpath);

    virtual ~ControlAcceptor();

    virtual ACE_HANDLE get_handle() const { return m_acceptor.get_handle(); }

    virtual int handle_input(ACE_HANDLE i_fd = ACE_INVALID_HANDLE);

    virtual int handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask);

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    void init();

    void open();

private:
    utp::FileSystemHandle	m_fsh;
    std::string				m_sockpath;
    ACE_Reactor *			m_reactor;
    ACE_LSOCK_Acceptor		m_acceptor;
    
};

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_ControlAcceptor_h__
