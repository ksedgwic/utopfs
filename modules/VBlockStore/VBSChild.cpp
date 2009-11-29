#include <ace/Reactor.h>

#include "BlockStoreFactory.h"
#include "BlockStore.h"
#include "Log.h"
#include "Stats.h"

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
    , m_getcount(0)
    , m_getbytes(0)
    , m_putcount(0)
    , m_putbytes(0)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");

    m_bsh = BlockStoreFactory::lookup(i_instname);

    m_bsh->bs_register_unsathandler(this, NULL);

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
VBSChild::uh_unsaturated(void const * i_argp)
{
    LOG(lgr, 6, m_instname << ' ' << "uh_unsaturated");
    initiate_requests();
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
VBSChild::report_get(size_t i_nbytes)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);
    ++m_getcount;
    m_getbytes += i_nbytes;
}

void
VBSChild::report_put(size_t i_nbytes)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);
    ++m_putcount;
    m_putbytes += i_nbytes;
}

void
VBSChild::get_stats(StatSet & o_ss) const
{
    o_ss.set_name(string("vbs.") + m_instname);

    size_t getq;
    size_t putq;
    size_t rfrq;
    size_t hedq;
    int64 nget;
    int64 getb;
    int64 nput;
    int64 putb;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);
        getq = m_getreqs.size();
        putq = m_putreqs.size();
        rfrq = m_refreqs.size();
        hedq = m_shereqs.size();
        nget = m_getcount;
        getb = m_getbytes;
        nput = m_putcount;
        putb = m_putbytes;
    }

    // Report queue lengths.
    Stats::set(o_ss, "gql", getq, 1.0, "%.0f", SF_VALUE);
    Stats::set(o_ss, "pql", putq, 1.0, "%.0f", SF_VALUE);
    Stats::set(o_ss, "rql", rfrq, 1.0, "%.0f", SF_VALUE);
    Stats::set(o_ss, "hql", hedq, 1.0, "%.0f", SF_VALUE);

    // Report operation rates.
    Stats::set(o_ss, "grps", nget, 1.0, "%.1f/s", SF_DELTA);
    Stats::set(o_ss, "gbps", getb, 1.0/1024.0, "%.1fKB/s", SF_DELTA);
    Stats::set(o_ss, "prps", nput, 1.0, "%.1f/s", SF_DELTA);
    Stats::set(o_ss, "pbps", putb, 1.0/1024.0, "%.1fKB/s", SF_DELTA);
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
        // Check to make sure the child isn't saturated.
        if (m_bsh->bs_issaturated())
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);
            m_notified = false;
            break;
        }

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
                LOG(lgr, 6, m_instname << ' '
                    << "initiate refresh request starting");
                rrh = m_refreqs.front();
                m_refreqs.pop_front();
            }
            else if (!m_shereqs.empty())
            {
                LOG(lgr, 6, m_instname << ' '
                    << "initiate headnode request starting");
                rrh = m_shereqs.front();
                m_shereqs.pop_front();
            }
            else if (!m_getreqs.empty())
            {
                LOG(lgr, 6, m_instname << ' '
                    << "initiate get request starting");
                grh = m_getreqs.front();
                m_getreqs.pop_front();
            }
            else if (!m_putreqs.empty())
            {
                LOG(lgr, 6, m_instname << ' '
                    << "initiate put request starting");
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
            LOG(lgr, 6, m_instname << ' '
                    << "initiate get request finished");
        }
        else if (prh)
        {
            prh->initiate(this, m_bsh);
            LOG(lgr, 6, m_instname << ' '
                    << "initiate put request finished");
        }
        else if (rrh)
        {
            rrh->initiate(this, m_bsh);
            LOG(lgr, 6, m_instname << ' '
                    << "initiate refresh/headnode request finished");
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
