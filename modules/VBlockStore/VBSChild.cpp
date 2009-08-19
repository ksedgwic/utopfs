#include <ace/Reactor.h>

#include "Log.h"
#include "BlockStore.h"
#include "BlockStoreFactory.h"

#include "VBSChild.h"
#include "vbslog.h"
#include "VBSRequest.h"

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
    process_requests();
    
    return 0;
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
VBSChild::process_requests()
{
    LOG(lgr, 4, m_instname << ' ' << "process_requests starting");

    while (true)
    {
        // First, figure out what we are going to do with the mutex
        // held.
        //
        VBSPutRequestHandle prh = NULL;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_chldmutex);

            if (!m_putreqs.empty())
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
        if (prh)
        {
            prh->process(this, m_bsh);
        }
        else
        {
            // We're done, break out of loop.
            break;
        }
    }

    LOG(lgr, 4, m_instname << ' ' << "process_requests finished");
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
