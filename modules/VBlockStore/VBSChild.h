#ifndef VBSChild_h__
#define VBSChild_h__

/// @file VBSChild.h
/// Virtual BlockStore VBSChild

#include <deque>
#include <string>

#include <ace/Event_Handler.h>
#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockStore.h"
#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore VBSChild
//
class VBS_EXP VBSChild
    : public ACE_Event_Handler
    , public utp::BlockStore::UnsaturatedHandler
    , public virtual utp::RCObj
{
public:
    VBSChild(ACE_Reactor * i_reactor,
             std::string const & i_instname);

    virtual ~VBSChild();

    // ACE_Event_Handler

    virtual Reference_Count add_reference();

    virtual Reference_Count remove_reference();

    virtual int handle_exception(ACE_HANDLE fd);

    // UnsaturatedHandler

    virtual void uh_unsaturated(void const * i_argp);

    // VBSChild

    std::string const & instname() const { return m_instname; }

    utp::BlockStoreHandle const & bs() const { return m_bsh; }

    void enqueue_get(VBSGetRequestHandle const & i_grh);

    void enqueue_put(VBSPutRequestHandle const & i_prh);

    void enqueue_refresh(VBSRequestHandle const & i_rh);

    void enqueue_headnode(VBSRequestHandle const & i_rh);

    VBSGetRequestHandle cancel_get(utp::OctetSeq const & i_key);

    void get_stats(utp::StatSet & o_ss) const;

protected:
    void initiate_requests();

private:
    ACE_Reactor *						m_reactor;
    std::string							m_instname;
    utp::BlockStoreHandle				m_bsh;

    mutable ACE_Thread_Mutex			m_chldmutex;
    bool								m_notified;
    std::deque<VBSGetRequestHandle>		m_getreqs;
    std::deque<VBSPutRequestHandle>		m_putreqs;
    std::deque<VBSRequestHandle>		m_refreqs;
    std::deque<VBSRequestHandle>		m_shereqs;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSChild_h__
