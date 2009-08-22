#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFurthestRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadFurthestRequest::VBSHeadFurthestRequest(VBlockStore & i_vbs,
                                           long i_outstanding,
                                           SignedHeadNode const & i_shn,
                                           BlockStore::SignedHeadTraverseFunc * i_cmpl,
                                           void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_shn(i_shn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "SHN FURTHEST @" << (void *) this << " CTOR");
}

VBSHeadFurthestRequest::~VBSHeadFurthestRequest()
{
    LOG(lgr, 6, "SHN FURTHEST @" << (void *) this << " DTOR");
}

void
VBSHeadFurthestRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "SHN FURTHEST @" << (void *) this;
}

void
VBSHeadFurthestRequest::initiate(VBSChild * i_cp,
                                 BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate");

    i_bsh->bs_head_furthest_async(m_shn, *this, i_cp);
}

void
VBSHeadFurthestRequest::sht_node(void const * i_argp,
                                 SignedHeadNode const & i_shn)
{
}

void
VBSHeadFurthestRequest::sht_complete(void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " shi_complete");

    bool do_complete = false;
    bool do_done = false;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the first successful completion?
        if (!m_succeeded)
        {
            do_complete = true;
            m_succeeded = true;
        }

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
            do_done = true;
    }

    // If we are the first child back with success we get
    // to tell the parent ...
    //
    if (do_complete && m_cmpl)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");
        m_cmpl->sht_complete(m_argp);
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
    {
        LOG(lgr, 6, *this << ' ' << "DONE");
        done();
    }
}

void
VBSHeadFurthestRequest::sht_error(void const * i_argp,
                                  utp::Exception const & i_exp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " shi_error");

    bool do_complete = false;
    bool do_done = false;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
        {
            do_done = true;

            // If no other child succeeded send our status.
            if (!m_succeeded)
                do_complete = true;
        }
    }

    // If we are the last child back with an exception we
    // get to tell the parent ...
    //
    if (do_complete && m_cmpl)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
        m_cmpl->sht_error(m_argp, i_exp);
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
    {
        LOG(lgr, 6, *this << ' ' << "DONE");
        done();
    }
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
