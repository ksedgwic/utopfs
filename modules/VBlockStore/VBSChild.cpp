#include <ace/Reactor.h>

#include "Log.h"
#include "BlockStore.h"
#include "BlockStoreFactory.h"

#include "VBSChild.h"
#include "vbslog.h"
#include "VBSRequest.h"
#include "VBSGetRequest.h"
#include "VBSPutRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSChild::VBSChild(ACE_Reactor * i_reactor, string const & i_instname)
    : m_reactor(i_reactor)
    , m_instname(i_instname)
    , m_notified(false)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");

    m_bsh = BlockStoreFactory::lookup(i_instname);

    this->reference_counting_policy().value
        (ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

VBSChild::~VBSChild()
{
    LOG(lgr, 4, m_instname << ' ' << "DTOR");
}

ACE_Event_Handler::Reference_Count
VBSChild::add_reference()
{
    LOG(lgr, 9, m_instname << ' ' << "add_reference "
        << rc_count() << "->" << (rc_count() + 1));

    return this->rc_add_ref();
}

ACE_Event_Handler::Reference_Count
VBSChild::remove_reference()
{
    LOG(lgr, 9, m_instname << ' ' << "remove_reference "
        << rc_count() << "->" << (rc_count() - 1));

    // Don't touch any members after this!
    return this->rc_rem_ref();
}

int
VBSChild::handle_exception(ACE_HANDLE fd)
{
    initiate_requests();
    
    return 0;
}

void
VBSChild::enqueue_get(VBSGetRequestHandle const & i_grh)
{
    LOG(lgr, 6, m_instname << ' ' << "enqueue_get " << *i_grh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    m_getreqs.push_back(i_grh);

    if (!m_notified)
    {
        m_reactor->notify(this);
        m_notified = true;
    }
}

void
VBSChild::enqueue_put(VBSPutRequestHandle const & i_prh)
{
    LOG(lgr, 6, m_instname << ' ' << "enqueue_put " << *i_prh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    m_putreqs.push_back(i_prh);

    if (!m_notified)
    {
        m_reactor->notify(this);
        m_notified = true;
    }
}

void
VBSChild::enqueue_refresh(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, m_instname << ' ' << "enqueue_refresh " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    m_refreqs.push_back(i_rh);

    if (!m_notified)
    {
        m_reactor->notify(this);
        m_notified = true;
    }
}

void
VBSChild::enqueue_headnode(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, m_instname << ' ' << "enqueue_headnode " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    m_shereqs.push_back(i_rh);

    if (!m_notified)
    {
        m_reactor->notify(this);
        m_notified = true;
    }
}

VBSGetRequestHandle
VBSChild::cancel_get(utp::OctetSeq const & i_key)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    VBSGetRequestHandle grh;

    // Scan the requests looking for a match on key.
    std::deque<VBSGetRequestHandle>::iterator it;
    for (it = m_getreqs.begin(); it != m_getreqs.end(); ++it)
    {
        if ((*it)->key() == i_key)
        {
            LOG(lgr, 6, m_instname << ' '
                << "cancel_get CANCELED: " << keystr(i_key));
            grh = *it;
            break;
        }
    }

    // Remove it from the dequeue.
    if (it != m_getreqs.end())
        m_getreqs.erase(it);

    return grh;
}

void
VBSChild::get_stats(StatSet & o_ss) const
{
    o_ss.set_name(m_instname);	// Likely redundant, child BS sets it too.

    size_t nget;
    size_t nput;
    size_t nrfr;
    size_t nhed;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);
        nget = m_getreqs.size();
        nput = m_putreqs.size();
        nrfr = m_refreqs.size();
        nhed = m_shereqs.size();
    }

    {
        StatRec * srp = o_ss.add_rec();
        srp->set_name("nget");
        srp->set_value(nget);
        StatFormat * sfp = srp->add_format();
        sfp->set_fmtstr("%.0f");
        sfp->set_fmttype(SF_VALUE);
    }
    {
        StatRec * srp = o_ss.add_rec();
        srp->set_name("nput");
        srp->set_value(nput);
        StatFormat * sfp = srp->add_format();
        sfp->set_fmtstr("%.0f");
        sfp->set_fmttype(SF_VALUE);
    }
    {
        StatRec * srp = o_ss.add_rec();
        srp->set_name("nrfr");
        srp->set_value(nrfr);
        StatFormat * sfp = srp->add_format();
        sfp->set_fmtstr("%.0f");
        sfp->set_fmttype(SF_VALUE);
    }
    {
        StatRec * srp = o_ss.add_rec();
        srp->set_name("nhed");
        srp->set_value(nhed);
        StatFormat * sfp = srp->add_format();
        sfp->set_fmtstr("%.0f");
        sfp->set_fmttype(SF_VALUE);
    }
}

void
VBSChild::initiate_requests()
{
    LOG(lgr, 6, m_instname << ' ' << "initiate_requests starting");

    // NOTE - This loop handles requests in the following order:
    //
    // 1) Refresh requests
    // 2) Signed HeadNode requests
    // 3) Get requests
    // 4) Put requests
    //
    // Maybe this is on purpose ...
    //
    while (true)
    {
        // First, figure out what we are going to do with the mutex
        // held.
        //
        VBSGetRequestHandle grh = NULL;
        VBSPutRequestHandle prh = NULL;
        VBSRequestHandle rrh = NULL;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

            if (!m_refreqs.empty())
            {
                rrh = m_refreqs.front();
                m_refreqs.pop_front();
            }
            else if (!m_shereqs.empty())
            {
                rrh = m_shereqs.front();
                m_shereqs.pop_front();
            }
            else if (!m_getreqs.empty())
            {
                grh = m_getreqs.front();
                m_getreqs.pop_front();
            }
            else if (!m_putreqs.empty())
            {
                prh = m_putreqs.front();
                m_putreqs.pop_front();
            }
            else
            {
                // If we get here we're done.
                m_notified = false;
            }
        }

        // Next, carry out the action w/o the mutex held.
        //
        if (grh)
        {
            grh->initiate(this, m_bsh);
        }
        else if (prh)
        {
            prh->initiate(this, m_bsh);
        }
        else if (rrh)
        {
            rrh->initiate(this, m_bsh);
        }
        else
        {
            // We're done, break out of loop.
            break;
        }
    }

    LOG(lgr, 6, m_instname << ' ' << "initiate_requests finished");
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
