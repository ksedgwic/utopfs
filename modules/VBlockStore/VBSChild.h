#ifndef VBSChild_h__
#define VBSChild_h__

/// @file VBSChild.h
/// Virtual BlockStore VBSChild

#include <deque>
#include <string>

#include <ace/Event_Handler.h>
#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore VBSChild
//
class VBS_EXP VBSChild
    : public ACE_Event_Handler
    , public virtual utp::RCObj
{
public:
    VBSChild(std::string const & i_instname);

    virtual ~VBSChild();

    // ACE_Event_Handler

    virtual Reference_Count add_reference();

    virtual Reference_Count remove_reference();

    virtual int handle_exception(ACE_HANDLE fd);

    // VBSChild

    utp::BlockStoreHandle const & bs() const { return m_bsh; }

    void enqueue_put(VBSPutRequestHandle const & i_prh);

protected:
    void process_requests();

private:
    std::string							m_instname;
    ACE_Reactor *						m_reactor;
    utp::BlockStoreHandle				m_bsh;

    ACE_Thread_Mutex					m_chldmutex;
    bool								m_notified;
    std::deque<VBSPutRequestHandle>		m_putreqs;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSChild_h__
