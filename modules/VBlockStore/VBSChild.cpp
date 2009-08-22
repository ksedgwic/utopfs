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

VBSChild::VBSChild(string const & i_instname)
    : m_instname(i_instname)
    , m_reactor(ACE_Reactor::instance())
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
    LOG(lgr, 4, m_instname << ' ' << "enqueue_get " << *i_grh);

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
    LOG(lgr, 4, m_instname << ' ' << "enqueue_put " << *i_prh);

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
    LOG(lgr, 4, m_instname << ' ' << "enqueue_refresh " << *i_rh);

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
    LOG(lgr, 4, m_instname << ' ' << "enqueue_headnode " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

    m_shnreqs.push_back(i_rh);

    if (!m_notified)
    {
        m_reactor->notify(this);
        m_notified = true;
    }
}

void
VBSChild::initiate_requests()
{
    LOG(lgr, 4, m_instname << ' ' << "initiate_requests starting");

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
            else if (!m_shnreqs.empty())
            {
                rrh = m_shnreqs.front();
                m_shnreqs.pop_front();
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

    LOG(lgr, 4, m_instname << ' ' << "initiate_requests finished");
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
