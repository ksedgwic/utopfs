#include <iostream>
#include <string>

#include "BlockStore.h"
#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFollowRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadFollowRequest::VBSHeadFollowRequest(VBSRequestHolder & i_vbs,
                                           long i_outstanding,
                                           HeadNode const & i_hn,
                                           BlockStore::HeadEdgeTraverseFunc * i_cmpl,
                                           void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_hn(i_hn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "FOLLOW @" << (void *) this << " CTOR");
}

VBSHeadFollowRequest::~VBSHeadFollowRequest()
{
    LOG(lgr, 6, "FOLLOW @" << (void *) this << " DTOR");
}

void
VBSHeadFollowRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "FOLLOW @" << (void *) this;
}

void
VBSHeadFollowRequest::initiate(VBSChild * i_cp,
                               BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    i_bsh->bs_head_follow_async(m_hn, *this, i_cp);
}

void
VBSHeadFollowRequest::het_edge(void const * i_argp,
                               SignedHeadEdge const & i_she)
{
    LameEdgeHandle leh = new LameEdge(i_she);

    m_edges.insert(leh);
}

void
VBSHeadFollowRequest::het_complete(void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " hnt_complete");

    m_succeeded = true;

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
            do_complete = true;
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowRequest::het_error(void const * i_argp,
                                Exception const & i_exp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " hnt_error");

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
        {
            do_complete = true;
        }
    }

    if (do_complete)
        complete();
}

void
VBSHeadFollowRequest::complete()
{
    if (!m_succeeded)
    {
        if (m_cmpl)
        {
            LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
            m_cmpl->het_error(m_argp,
                              NotFoundError("no starting seed found"));
        }
    }
    else
    {
        if (m_cmpl)
        {
            LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");
            for (LameEdgeSet::const_iterator it = m_edges.begin();
                 it != m_edges.end();
                 ++it)
                m_cmpl->het_edge(m_argp, (*it)->m_she);

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
