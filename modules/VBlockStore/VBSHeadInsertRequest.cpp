#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadInsertRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadInsertRequest::VBSHeadInsertRequest(VBlockStore & i_vbs,
                                           long i_outstanding,
                                           SignedHeadEdge const & i_she,
                                           BlockStore::HeadEdgeInsertCompletion * i_cmpl,
                                           void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_she(i_she)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "INSERT @" << (void *) this << " CTOR");
}

VBSHeadInsertRequest::~VBSHeadInsertRequest()
{
    LOG(lgr, 6, "INSERT @" << (void *) this << " DTOR");
}

void
VBSHeadInsertRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "INSERT @" << (void *) this;
}

void
VBSHeadInsertRequest::initiate(VBSChild * i_cp,
                               BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    i_bsh->bs_head_insert_async(m_she, *this, i_cp);
}

void
VBSHeadInsertRequest::hei_complete(SignedHeadEdge const & i_she,
                                   void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " hei_complete");

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
        m_cmpl->hei_complete(m_she, m_argp);
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
VBSHeadInsertRequest::hei_error(SignedHeadEdge const & i_she,
                                void const * i_argp,
                                Exception const & i_exp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " hei_error");

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
        m_cmpl->hei_error(m_she, m_argp, i_exp);
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
