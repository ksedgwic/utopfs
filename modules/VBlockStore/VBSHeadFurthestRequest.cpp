#include <iostream>
#include <string>

#include "BlockStore.h"
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
                                               HeadNode const & i_hn,
                                               BlockStore::HeadNodeTraverseFunc * i_cmpl,
                                               void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_hn(i_hn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
    , m_firsttry(true)
{
    LOG(lgr, 6, "FURTHEST @" << (void *) this << " CTOR");
}

VBSHeadFurthestRequest::~VBSHeadFurthestRequest()
{
    LOG(lgr, 6, "FURTHEST @" << (void *) this << " DTOR");
}

void
VBSHeadFurthestRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "FURTHEST @" << (void *) this;
}

void
VBSHeadFurthestRequest::initiate(VBSChild * i_cp,
                                 BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate");

    i_bsh->bs_head_furthest_async(m_hn, *this, i_cp);
}

void
VBSHeadFurthestRequest::hnt_node(void const * i_argp,
                                 HeadNode const & i_hn)
{
    VBSChild * cp = (VBSChild *) i_argp;

    m_cnsm[cp].insert(i_hn);
}

void
VBSHeadFurthestRequest::hnt_complete(void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " hnt_complete");

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
VBSHeadFurthestRequest::hnt_error(void const * i_argp,
                                  utp::Exception const & i_exp)
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
VBSHeadFurthestRequest::complete()
{
    bool do_done = false;

    if (m_cnsm.empty())
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
        if (m_cmpl)
            m_cmpl->hnt_error(m_argp,
                              NotFoundError("no starting seed found"));

        do_done = true;
    }
    else
    {
        // Compute the intersection of all children's sets of
        // furthest nodes (the set all children have).
        //
        size_t nchild = m_cnsm.size(); // How many kids had sets?
        ChildNodeSetMap::const_iterator it = m_cnsm.begin();
        HeadNodeSeq inter(it->second.begin(), it->second.end());
        for (++it; it!= m_cnsm.end(); ++it)
        {
            HeadNodeSeq i0(it->second.begin(), it->second.end());
            HeadNodeSeq i1;
            set_intersection(i0.begin(), i0.end(),
                             inter.begin(), inter.end(),
                             back_inserter(i1));
            inter = i1;
        }

        // For each of the children, compute the unique nodes.
        bool nodiff = true;
        for (it = m_cnsm.begin(); it != m_cnsm.end(); ++it)
        {
            HeadNodeSeq diff;
            HeadNodeSet const & hns = it->second;
            set_difference(hns.begin(), hns.end(),
                           inter.begin(), inter.end(),
                           back_inserter(diff));
            if (!diff.empty())
            {
                nodiff = false;

                LOG(lgr, 6, "CHILD " << it->first->instname()
                    << " HAS UNIQUE NODES:");
                for (unsigned ii = 0; ii < diff.size(); ++ii)
                {
                    utp::HeadNode const & hn = diff[ii];
                    LOG(lgr, 6, hn);

#warning "FIXME"
#if 0
                    // We really only need to send the follow to the
                    // other children, and we really only need to
                    // insert in this child.
                    VBSHeadFillRequestHandle hfrh =
                        new VBSHeadFillRequest(*this,
                                               nchild - 1,
                                               hn,
                                               NULL,
                                               NULL,
                                               it->first);

                    {
                        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);
                        m_subreqs.insert(hfrh);
                    }

                    // Loop over all the other kids queuing this
                    // fill requests.
                    //
                    for (ChildNodeSetMap::const_iterator it2 = m_cnsm.begin();
                         it2 != m_cnsm.end();
                         ++it2)
                    {
                        if (it2->first != it->first)
                            it2->first->enqueue_headnode(hfrh);
                    }
#endif
                }
            }
        }

        if (nodiff)
        {
            if (m_cmpl)
            {
                LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");
                for (unsigned ii = 0; ii < inter.size(); ++ii)
                    m_cmpl->hnt_node(m_argp, inter[ii]);
                m_cmpl->hnt_complete(m_argp);
            }
            do_done = true;
        }
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
