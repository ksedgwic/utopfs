#include <iostream>
#include <string>

#include "BlockStore.h"
#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFurthestTopReq.h"
#include "VBSHeadFurthestSubReq.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSHeadFurthestTopReq::VBSHeadFurthestTopReq(VBSRequestHolder & i_vbs,
                                             long i_outstanding,
                                             HeadNode const & i_hn,
                                             BlockStore::HeadNodeTraverseFunc * i_cmpl,
                                             void const * i_argp,
                                             VBSChildMap const & i_children)
    : VBSRequest(i_vbs, i_outstanding)
    , m_hn(i_hn)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
    , m_children(i_children)
{
    LOG(lgr, 6, "FURTHEST TOP @" << (void *) this << " CTOR");
}

VBSHeadFurthestTopReq::~VBSHeadFurthestTopReq()
{
    LOG(lgr, 6, "FURTHEST TOP @" << (void *) this << " DTOR");
}

void
VBSHeadFurthestTopReq::stream_insert(std::ostream & ostrm) const
{
    ostrm << "FURTHEST TOP @" << (void *) this;
}

void
VBSHeadFurthestTopReq::initiate(VBSChild * i_cp,
                                BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    i_bsh->bs_head_furthest_async(m_hn, *this, i_cp);
}

void
VBSHeadFurthestTopReq::rh_insert(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, "rh_insert " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_hftrmutex);

    m_subreqs.insert(i_rh);
}

void
VBSHeadFurthestTopReq::rh_remove(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, "rh_remove " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_hftrmutex);

    // Erase this request from the set.
    size_t nrm = m_subreqs.erase(i_rh);
    if (nrm != 1)
        throwstream(InternalError, FILELINE
                    << "expected to remove one request, removed " << nrm);

#if 0
    // If we've emptied the request list wake any waiters.
    if (m_subreqs.empty() && m_waiting)
    {
        m_vbscond.broadcast();
        m_waiting = false;
    }
#endif
}

void
VBSHeadFurthestTopReq::hnt_node(void const * i_argp,
                                HeadNode const & i_hn)
{
    // Don't think this gets called.
    throwstream(InternalError, FILELINE
                << "VBSHeadFurthestTopReq::hnt_node shouldn't get called");
}

void
VBSHeadFurthestTopReq::hnt_complete(void const * i_argp)
{
    long val = (long) i_argp;

    LOG(lgr, 6, "hnt_complete " << val);

    bool is_done = false;

    switch (val)
    {
    case 0:
        // This was the initial "survey" further request.  Did we get
        // an answer that is immediately useful?
        //
        if (m_subfurther->unique().empty())
        {
            if (m_cmpl)
            {
                LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");

                // Yes, there are no unique child answers.
                HeadNodeSeq const & hns = m_subfurther->common();
                for (unsigned ii = 0; ii < hns.size(); ++ii)
                    m_cmpl->hnt_node(m_argp, hns[ii]);

                m_cmpl->hnt_complete(m_argp);
            }
            is_done = true;

            // Release our ref to this.
            m_subfurther = NULL;
        }
        else
        {
            // We've got children with unique answers, try and
            // clean this up with follow fills ...
            //
            LOG(lgr, 6, *this << ' ' << "RESOLVING UNIQUE CHILDREN");
        }
        break;
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (is_done)
    {
        LOG(lgr, 6, *this << ' ' << "DONE");
        done();
    }
}

void
VBSHeadFurthestTopReq::hnt_error(void const * i_argp,
                                 utp::Exception const & i_exp)
{
    long val = (long) i_argp;

    LOG(lgr, 6, "hnt_error " << val);

    if (m_cmpl)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
        m_cmpl->hnt_error(m_argp, i_exp);
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    LOG(lgr, 6, *this << ' ' << "DONE");
    done();
}

void
VBSHeadFurthestTopReq::init()
{
    // NOTE - We initiate the sequence with a simple furthest
    // subrequest.  If there are no child-unique answers we're done!

    // Create a sub-request and send it to the children
    m_subfurther =
        new VBSHeadFurthestSubReq(*this,
                                  m_children.size(),
                                  m_hn,
                                  this,
                                  (void *) 0);

    LOG(lgr, 6, "bs_head_further_async " << *m_subfurther);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    rh_insert(m_subfurther);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_headnode(m_subfurther);
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
