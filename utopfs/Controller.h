#ifndef Controller_h__
#define Controller_h__

/// @file Controller.h
/// Utopia FileSystem Controller Object.
///
/// Controller for Utopfs FileSystem.

#include <string>

#include <ace/Event_Handler.h>
#include <ace/LSOCK_Acceptor.h>

#include "Assembly.h"
#include "utpfwd.h"

class Controller : public ACE_Event_Handler
{
public:
    Controller(utp::Assembly * i_ap,
               std::string const & i_controlpath,
               double i_syncsecs);

    virtual ~Controller();

    virtual ACE_HANDLE get_handle() const { return m_acceptor.get_handle(); }

    virtual int handle_input(ACE_HANDLE i_fd = ACE_INVALID_HANDLE);

    virtual int handle_close(ACE_HANDLE i_handle, ACE_Reactor_Mask i_mask);

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    void init();

    void term();

    void do_open();

    void do_sync();

private:
    bool						m_opened;
    std::string					m_sockpath;
    utp::Assembly *				m_ap;
    utp::BlockStoreHandle		m_bsh;
    utp::FileSystemHandle		m_fsh;
    std::string					m_controlpath;
    double						m_syncsecs;
    ACE_Reactor *				m_reactor;
    ACE_LSOCK_Acceptor			m_acceptor;

    mutable ACE_Thread_Mutex	m_cntrlmutex;
    bool						m_issyncing;
};

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_Controller_h__
