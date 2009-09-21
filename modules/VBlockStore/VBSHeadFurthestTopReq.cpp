#include <iostream>
#include <string>

#include "BlockStore.h"
#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSHeadFurthestTopReq.h"
#include "VBSHeadFurthestSubReq.h"
#include "VBSHeadFollowFillReq.h"
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
    , m_lasttry(false)
    , m_ffout(0)
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
VBSHeadFurthestTopReq::het_edge(void const * i_argp,
                                SignedHeadEdge const & i_she)
{
    // This should never be called.
    throwstream(InternalError, FILELINE << "shouldn't be here");
}

void
VBSHeadFurthestTopReq::het_complete(void const * i_argp)
{
    LOG(lgr, 6, *this << " het_complete");

    m_succeeded = true;

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        if (--m_ffout == 0)
            do_complete = true;
    }

    if (do_complete)
    {
        if (!m_lasttry)
            second_check();
        else
            last_check();
    }
}

void
VBSHeadFurthestTopReq::het_error(void const * i_argp,
                                 Exception const & i_exp)
{
    LOG(lgr, 6, *this << " het_error");

    // Unlike the other completion callbacks this one is not
    // complete until all of the children have checked in.

    bool do_complete = false;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        if (--m_ffout == 0)
            do_complete = true;
    }

    if (do_complete)
    {
        if (!m_lasttry)
            second_check();
        else
            last_check();
    }
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

    // Take the reference out of the member variable; it gets
    // used by subroutines we call ...
    //
    VBSHeadFurthestSubReqHandle subfurther = m_subfurther;
    m_subfurther = NULL;

    LOG(lgr, 6, "hnt_complete " << val);

    bool is_done = false;

    switch (val)
    {
    case 0:
        // This was the initial "survey" further request.  Did we get
        // an answer that is immediately useful?
        //
        if (subfurther->unique().empty())
        {
            if (m_cmpl)
            {
                LOG(lgr, 6, *this << ' ' << "INITIAL FURTHEST GOOD");

                // Yes, there are no unique child answers.
                HeadNodeSeq const & hns = subfurther->common();
                for (unsigned ii = 0; ii < hns.size(); ++ii)
                    m_cmpl->hnt_node(m_argp, hns[ii]);

                m_cmpl->hnt_complete(m_argp);
            }
            is_done = true;
        }
        else
        {
            // We've got children with unique answers, try and
            // clean this up with follow fills ...
            //
            LOG(lgr, 6, *this << ' ' << "RESOLVING UNIQUE CHILDREN");

            directed_follow_fill(subfurther);

            is_done = false;
        }
        break;

    case 1:
        // This was the second "survey" further request.  Did
        // the directed follow-fill anneal the unique nodes?
        //
        if (subfurther->unique().empty())
        {
            if (m_cmpl)
            {
                LOG(lgr, 6, *this << ' ' << "DIRECTED FOLLOWFILL WORKED");

                // Yes, there are no unique child answers.
                HeadNodeSeq const & hns = subfurther->common();
                for (unsigned ii = 0; ii < hns.size(); ++ii)
                    m_cmpl->hnt_node(m_argp, hns[ii]);

                m_cmpl->hnt_complete(m_argp);
            }
            is_done = true;
        }
        else
        {
            // We've got children with unique answers, try and
            // clean this up with follow fills ...
            //
            LOG(lgr, 6, *this << ' ' << "DESPERATELY TRYING FULL FOLLOWFILL");

            full_follow_fill();

            is_done = false;
        }
        break;

    case 2:
        // This was the last-ditch full follow-fill.
        //
        // Something is very wrong if it doesn't work since
        // all head nodes are copied.
        //
        if (subfurther->unique().empty())
        {
            if (m_cmpl)
            {
                LOG(lgr, 6, *this << ' ' << "FULL FOLLOWFILL WORKED");

                // Yes, there are no unique child answers.
                HeadNodeSeq const & hns = subfurther->common();
                for (unsigned ii = 0; ii < hns.size(); ++ii)
                    m_cmpl->hnt_node(m_argp, hns[ii]);

                m_cmpl->hnt_complete(m_argp);
            }
            is_done = true;
        }
        else
        {
            // This shouldn't happen!
            //
            throwstream(InternalError, FILELINE
                        << "FULL FOLLOWFILL DIDN'T WORK");
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
    //
    // We pass a '0' as the argp to discriminate this call from
    // some later ones.

    // Create a sub-request and send it to the children
    m_subfurther =
        new VBSHeadFurthestSubReq(m_vbs,
                                  m_children.size(),
                                  m_hn,
                                  this,
                                  (void *) 0);

    LOG(lgr, 6, "bs_head_further_async " << *m_subfurther);

    // Need to do this *before* we enqueue the requests.
    m_vbs.rh_insert(m_subfurther);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_headnode(m_subfurther);
}

void
VBSHeadFurthestTopReq::directed_follow_fill
					(VBSHeadFurthestSubReqHandle const & i_srh)
{
    // Our initial survey showed that some of the children have unique
    // further values.
    //
    // We're hoping that these unique values are just part of the
    // current history of one of the other kids.  In this case we can
    // "fast-forward" the unique value to one which everyone else has.
    //
    // Submit follow fill requests for these values to the other
    // children.  Insert all of the returned edges to the original
    // child.

    // Each FollowFill is submitted to all other children.
    size_t ncp = m_children.size() - 1;

    ChildNodeSetMap const & unq = i_srh->unique();
    for (ChildNodeSetMap::const_iterator it = unq.begin();
         it != unq.end();
         ++it)
    {
        VBSChild * cp = it->first;
        HeadNodeSet const & hns = it->second;

        for (HeadNodeSet::const_iterator it2 = hns.begin();
             it2 != hns.end();
             ++it2)
        {
            HeadNode const & hn = *it2;
            
            ++m_ffout;

            VBSRequestHandle rh =
                new VBSHeadFollowFillReq(m_vbs, ncp, hn, this, NULL, cp);

            // Need to do this before we enqueue.
            m_vbs.rh_insert(rh);

            // Enqueue the request w/ all of the except the one.
            for (VBSChildMap::const_iterator it3 = m_children.begin();
                 it3 != m_children.end();
                 ++it3)
                if (&*(it3->second) != cp)
                    it3->second->enqueue_headnode(rh);
        }
    }
}

void
VBSHeadFurthestTopReq::second_check()
{
    // We've completed the follow-fill pass after the initial unique
    // nodes were discovered.  Now we do another furthest pass to see
    // if the unique nodes have been annealed.
    //
    // This time we pass a '1' as the argp ...

    // Create a sub-request and send it to the children
    m_subfurther =
        new VBSHeadFurthestSubReq(m_vbs,
                                  m_children.size(),
                                  m_hn,
                                  this,
                                  (void *) 1);

    LOG(lgr, 6, "bs_head_further_async " << *m_subfurther);

    // Need to do this *before* we enqueue the requests.
    m_vbs.rh_insert(m_subfurther);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_headnode(m_subfurther);
}

void
VBSHeadFurthestTopReq::full_follow_fill()
{
    // Te directed follow-fill didn't work.  We issue a full
    // follow fill to copy all head entries.  This is expensive but
    // it is guranteed to work.

    m_lasttry = true;
    m_ffout = 0;

    // Each FollowFill is submitted to all other children.
    size_t ncp = m_children.size() - 1;

    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
    {
        VBSChildHandle ch = it->second;

        ++m_ffout;

        HeadNode hn(m_hn.first, "");

        VBSRequestHandle rh =
            new VBSHeadFollowFillReq(m_vbs, ncp, hn, this, NULL, &*ch);

        // Need to do this before we enqueue.
        m_vbs.rh_insert(rh);

        // Enqueue the request w/ all of the except the one.
        for (VBSChildMap::const_iterator it3 = m_children.begin();
             it3 != m_children.end();
             ++it3)
            if (&*(it3->second) != &*ch)
                it3->second->enqueue_headnode(rh);
    }
}

void
VBSHeadFurthestTopReq::last_check()
{
    // We've completed the last-ditch full follow-fill cycle to
    // copy everything.  Did it work?
    //
    // This time we pass a '2' as the argp ...

    // Create a sub-request and send it to the children
    m_subfurther =
        new VBSHeadFurthestSubReq(m_vbs,
                                  m_children.size(),
                                  m_hn,
                                  this,
                                  (void *) 2);

    LOG(lgr, 6, "bs_head_further_async " << *m_subfurther);

    // Need to do this *before* we enqueue the requests.
    m_vbs.rh_insert(m_subfurther);

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
