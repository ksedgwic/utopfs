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
#include "Stats.h"

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
    VBSChild(VBlockStore & i_vbs,
             ACE_Reactor * i_reactor,
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

    void report_get(size_t i_nbytes);

    void report_put(size_t i_nbytes);

    void get_stats(utp::StatSet & o_ss) const;

    void needed_keys_append(void const * i_keydata, size_t i_keysize);

    size_t needed_keys_size();

protected:
    void initiate_requests();

private:
    typedef std::deque<utp::OctetSeq> KeyQueue;

    VBlockStore &						m_vbs;
    ACE_Reactor *						m_reactor;
    std::string							m_instname;
    utp::BlockStoreHandle				m_bsh;

    mutable ACE_Thread_Mutex			m_chldmutex;
    bool								m_notified;
    std::deque<VBSGetRequestHandle>		m_getreqs;	// Get Request Queue
    std::deque<VBSPutRequestHandle>		m_putreqs;	// Put Request Queue
    std::deque<VBSRequestHandle>		m_refreqs;	// Refresh Request Queue
    std::deque<VBSRequestHandle>		m_shereqs;	// Signed Headnode Request Queue

    KeyQueue							m_neededkeys;

    utp::int64							m_getcount;
    utp::int64							m_getbytes;
    utp::int64							m_putcount;
    utp::int64							m_putbytes;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSChild_h__
