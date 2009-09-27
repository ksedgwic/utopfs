#include <iostream>
#include <string>

#include "BlockStore.h"
#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFollowFillReq.h"
#include "VBSHeadInsertRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadFollowFillReq::VBSHeadFollowFillReq(VBlockStore & i_vbs,
                                           long i_outstanding,
                                           HeadNode const & i_hn,
                                           BlockStore::HeadEdgeTraverseFunc * i_cmpl,
                                           void const * i_argp,
                                           VBSChild * i_targcp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_hn(i_hn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
    , m_targcp(i_targcp)
    , m_travdone(false)
    , m_fillsout(0)
{
    LOG(lgr, 6, "FOLLOWFILL @" << (void *) this << " CTOR");
}

VBSHeadFollowFillReq::~VBSHeadFollowFillReq()
{
    LOG(lgr, 6, "FOLLOWFILL @" << (void *) this << " DTOR");
}

void
VBSHeadFollowFillReq::stream_insert(std::ostream & ostrm) const
{
    ostrm << "FOLLOWFILL @" << (void *) this;
}

void
VBSHeadFollowFillReq::initiate(VBSChild * i_cp,
                               BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    i_bsh->bs_head_follow_async(m_hn, *this, i_cp);
}

void
VBSHeadFollowFillReq::het_edge(void const * i_argp,
                               SignedHeadEdge const & i_she)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " het_edge " << i_she);

    ++m_fillsout;

    // We only need to submit this fill request to the one
    // target child.
    //
    VBSRequestHandle rh =
        new VBSHeadInsertRequest(m_vbs, 1, i_she, this, NULL);

    // Enqueue on our parent's request list.
    m_vbs.insert_req(rh);

    m_targcp->enqueue_headnode(rh);
}

void
VBSHeadFollowFillReq::het_complete(void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " het_complete");

    m_succeeded = true;

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last traversal completion?
        --m_outstanding;
        if (m_outstanding == 0)
        {
            // The traversal is now complete.
            m_travdone = true;

            // But we're only complete when all of the fills are
            // complete as well.
            //
            if (m_fillsout.value() == 0)
                do_complete = true;
        }
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowFillReq::het_error(void const * i_argp,
                                Exception const & i_exp)
{
    LOG(lgr, 6, *this << " het_error");

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
        {
            // The traversal is now complete.
            m_travdone = true;

            // But we're only complete when all of the fills are
            // complete as well.
            //
            if (m_fillsout.value() == 0)
                do_complete = true;
        }
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowFillReq::hei_complete(SignedHeadEdge const & i_she,
                                   void const * i_argp)
{
    LOG(lgr, 6, *this << " hei_complete");

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // One less fill outstanding.
        --m_fillsout;

        if (m_travdone && m_fillsout.value() == 0)
            do_complete = true;
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowFillReq::hei_error(SignedHeadEdge const & i_she,
                                void const * i_argp,
                                Exception const & i_exp)
{
    LOG(lgr, 6, *this << " hei_error");

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // One less fill outstanding.
        --m_fillsout;

        if (m_travdone && m_fillsout.value() == 0)
            do_complete = true;
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowFillReq::complete()
{
    if (!m_succeeded)
    {
        if (m_cmpl)
        {
            LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");

            // Is this the right return exception?  Seems wrong ...
            m_cmpl->het_error(m_argp,
                              NotFoundError("no starting seed found"));
        }
    }
    else
    {
        if (m_cmpl)
        {
            LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");

            // We don't bother making the edge calls, our job was
            // to issue the fills ...

            m_cmpl->het_complete(m_argp);
        }
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    LOG(lgr, 6, *this << ' ' << "DONE");
    done();
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
