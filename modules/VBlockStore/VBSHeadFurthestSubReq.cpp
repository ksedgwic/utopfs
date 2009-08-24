#include <iostream>
#include <string>

#include "BlockStore.h"
#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFurthestSubReq.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadFurthestSubReq::VBSHeadFurthestSubReq(VBSRequestHolder & i_vbs,
                                             long i_outstanding,
                                             HeadNode const & i_hn,
                                             BlockStore::HeadNodeTraverseFunc * i_cmpl,
                                             void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_hn(i_hn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "FURTHEST SUB @" << (void *) this << " CTOR");
}

VBSHeadFurthestSubReq::~VBSHeadFurthestSubReq()
{
    LOG(lgr, 6, "FURTHEST SUB @" << (void *) this << " DTOR");
}

void
VBSHeadFurthestSubReq::stream_insert(std::ostream & ostrm) const
{
    ostrm << "FURTHEST SUB @" << (void *) this;
}

void
VBSHeadFurthestSubReq::initiate(VBSChild * i_cp,
                                BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    i_bsh->bs_head_furthest_async(m_hn, *this, i_cp);
}

void
VBSHeadFurthestSubReq::hnt_node(void const * i_argp,
                                HeadNode const & i_hn)
{
    VBSChild * cp = (VBSChild *) i_argp;

    m_cnsm[cp].insert(i_hn);
}

void
VBSHeadFurthestSubReq::hnt_complete(void const * i_argp)
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
VBSHeadFurthestSubReq::hnt_error(void const * i_argp,
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
VBSHeadFurthestSubReq::complete()
{
    if (m_cnsm.empty())
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
        if (m_cmpl)
            m_cmpl->hnt_error(m_argp,
                              NotFoundError("no starting seed found"));
    }
    else
    {
        // Compute the intersection of all children's sets of
        // furthest nodes (the set all children have).
        //
        ChildNodeSetMap::const_iterator it = m_cnsm.begin();
        m_cmnnodes.assign(it->second.begin(), it->second.end());
        for (++it; it!= m_cnsm.end(); ++it)
        {
            HeadNodeSeq i0(it->second.begin(), it->second.end());
            HeadNodeSeq i1;
            set_intersection(i0.begin(), i0.end(),
                             m_cmnnodes.begin(), m_cmnnodes.end(),
                             back_inserter(i1));
            m_cmnnodes = i1;
        }

        // For each of the children, compute the unique nodes.
        for (it = m_cnsm.begin(); it != m_cnsm.end(); ++it)
        {
            HeadNodeSeq diff;
            HeadNodeSet const & hns = it->second;
            set_difference(hns.begin(), hns.end(),
                           m_cmnnodes.begin(), m_cmnnodes.end(),
                           back_inserter(diff));
            if (!diff.empty())
            {
                LOG(lgr, 6, "CHILD " << it->first->instname()
                    << " HAS UNIQUE NODES:");
                for (unsigned ii = 0; ii < diff.size(); ++ii)
                {
                    utp::HeadNode const & hn = diff[ii];
                    LOG(lgr, 6, hn);
                }

                m_unqnodes[it->first].insert(diff.begin(), diff.end());
            }
        }

        // NOTE - This subrequest type doesn't bother making hnt_node
        // calls; we presume the parent request will access the common
        // and unique collections directly ...
        //
        LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");
        m_cmpl->hnt_complete(m_argp);
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
