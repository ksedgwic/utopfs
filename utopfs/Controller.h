#ifndef Controller_h__
#define Controller_h__

/// @file Controller.h
/// Utopia FileSystem Controller Object.
///
/// Controller for Utopfs FileSystem.

#include <string>

#include <ace/Event_Handler.h>

#include "utpfwd.h"

class Controller : public ACE_Event_Handler
{
public:
    Controller(std::string const & i_sockpath);

    virtual ~Controller();

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    void init();

private:
    std::string			m_sockpath;
    ACE_Reactor *		m_reactor;

};

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_Controller_h__
