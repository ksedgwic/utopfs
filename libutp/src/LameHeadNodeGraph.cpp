#include <deque>
#include <iostream>

#include "LameHeadNodeGraph.h"
#include "utplog.h"

using namespace std;
using namespace utp;

namespace utp {

typedef std::deque<LameEdgeHandle> LameEdgeSeq;

LameEdge::LameEdge(utp::SignedHeadEdge const & i_she)
    : m_she(i_she)
{
    HeadEdge he;
    if (!he.ParseFromString(i_she.headedge()))
        throwstream(InternalError, FILELINE << "failed to parse headedge");

    m_prev = make_pair(he.fstag(), he.prevref());
    m_root = make_pair(he.fstag(), he.rootref());
}

bool
LameEdge::operator<(LameEdge const & i_o) const
{
    return m_root != i_o.m_root ?
        m_root < i_o.m_root :
        m_prev < i_o.m_prev;
}

ostream &
operator<<(ostream & ostrm, LameEdge const & i_e)
{
    ostrm << i_e.m_prev << " -> " << i_e.m_root;
    return ostrm;
}

void
LameHeadNodeGraph::head_follow_async(HeadNode const & i_hn,
                                     BlockStore::HeadEdgeTraverseFunc & i_func,
                                     void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, "follow " << i_hn);

    bool none_found = false;
    LameEdgeSeq found;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_lhnmutex);

        HeadNode const & seed = i_hn;

        // Expand the seeds.
        HeadNodeSet seeds;
        if (seed.second.size() == 0)
        {
            // There was no reference, we should use all roots w/ the
            // same fstag.
            for (HeadNodeSet::const_iterator it = m_roots.lower_bound(seed);
                 it != m_roots.end() && it->first == seed.first;
                 ++it)
            {
                HeadNode nr = *it;
                seeds.insert(nr);
            }
        }
        else
        {
            // Yes, see if we can find the seed in the rootmap
            LameEdgeMap::const_iterator pos = m_rootmap.find(seed);
            if (pos != m_rootmap.end())
            {
                // It's here, we can start with it.
                seeds.insert(pos->first);
            }
            else
            {
                // Can we find children of this node instead?
                LameEdgeMap::const_iterator it = m_prevmap.lower_bound(seed);
                LameEdgeMap::const_iterator end = m_prevmap.upper_bound(seed);
                for (; it != end; ++it)
                {
                    seeds.insert(it->second->m_root);

                    // We need to call the traverse function on the
                    // children as well since they follow the seed.
                    //
                    LOG(lgr, 6, "edge " << it->second->m_she);
                    found.push_back(it->second);
                }
            }
        }

        // If our seeds collection is empty, we are out of luck.
        if (seeds.empty())
            none_found = true;

        // Replace elements of the seed set w/ their children.
        bool done ;
        do
        {
            // Start optimistically
            done = true;

            // Find a seed with children.
            for (HeadNodeSet::iterator it = seeds.begin();
                 it != seeds.end();
                 ++it)
            {
                HeadNode nr = *it;

                // Find all the children of this seed.
                HeadNodeSet kids;
                LameEdgeMap::iterator kit = m_prevmap.lower_bound(nr);
                LameEdgeMap::iterator end = m_prevmap.upper_bound(nr);
                for (; kit != end; ++kit)
                {
                    kids.insert(kit->second->m_root);

                    // Call the traverse function on the kid.
                    LOG(lgr, 6, "edge " << kit->second->m_she);
                    found.push_back(kit->second);
                }

                // Were there kids?
                if (!kids.empty())
                {
                    // Remove the parent from the set.
                    seeds.erase(nr);

                    // Insert the kids instead.
                    seeds.insert(kids.begin(), kids.end());

                    // Start over, we're not done.
                    done = false;
                    break;
                }
            }
        }
        while (!done);
    }

    // Now, with the lock no longer held make all of the completion
    // callbacks.
    //
    if (none_found)
    {
        i_func.het_error(i_argp, NotFoundError("no starting seed found"));
    }
    else
    {
        for (LameEdgeSeq::const_iterator it = found.begin();
             it != found.end();
             ++it)
            i_func.het_edge(i_argp, (*it)->m_she);
        
        i_func.het_complete(i_argp);
    }
}

void
LameHeadNodeGraph::head_furthest_async(HeadNode const & i_hn,
                                       BlockStore::HeadNodeTraverseFunc & i_func,
                                       void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, "furthest " << i_hn);

    bool none_found = false;
    LameEdgeSeq found;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_lhnmutex);

        HeadNode const & seed = i_hn;

        // Expand the seeds.
        HeadNodeSet seeds;
        if (seed.second.size() == 0)
        {
            // There was no reference, we should use all roots w/ the
            // same fstag.
            //
            for (HeadNodeSet::const_iterator it = m_roots.lower_bound(seed);
                 it != m_roots.end() && it->first == seed.first;
                 ++it)
            {
                HeadNode nr = *it;
                seeds.insert(nr);
            }
        }
        else
        {
            // Yes, see if we can find the seed in the rootmap
            LameEdgeMap::const_iterator pos = m_rootmap.find(seed);
            if (pos != m_rootmap.end())
            {
                // It's here, we can start with it.
                seeds.insert(pos->first);
            }
            else
            {
                // Can we find children of this node instead?
                LameEdgeMap::const_iterator it = m_prevmap.lower_bound(seed);
                LameEdgeMap::const_iterator end = m_prevmap.upper_bound(seed);
                for (; it != end; ++it)
                    seeds.insert(it->second->m_root);
            }
        }

        // If our seeds collection is empty, we are out of luck.
        if (seeds.empty())
            none_found = true;

        // Replace elements of the seed set w/ their children.
        bool done ;
        do
        {
            // Start optimistically
            done = true;

            // Find a seed with children.
            for (HeadNodeSet::iterator it = seeds.begin();
                 it != seeds.end();
                 ++it)
            {
                HeadNode nr = *it;

                // Find all the children of this seed.
                HeadNodeSet kids;
                LameEdgeMap::iterator kit = m_prevmap.lower_bound(nr);
                LameEdgeMap::iterator end = m_prevmap.upper_bound(nr);
                for (; kit != end; ++kit)
                {
                    LOG(lgr, 8, "adding kid " << *kit->second);
                    kids.insert(kit->second->m_root);
                }

                // Were there kids?
                if (!kids.empty())
                {
                    // Remove the parent from the set.
                    LOG(lgr, 8, "removing parent " << nr);
                    seeds.erase(nr);

                    // Insert the kids instead.
                    seeds.insert(kids.begin(), kids.end());

                    // Start over, we're not done.
                    done = false;
                    break;
                }
            }
        }
        while (!done);

        // Call the traversal function on each remaining node.
        for (HeadNodeSet::iterator it = seeds.begin(); it != seeds.end(); ++it)
        {
            LameEdgeMap::const_iterator pos = m_rootmap.find(*it);
            if (pos == m_rootmap.end())
                throwstream(InternalError, FILELINE
                            << "missing rootmap entry");

            // We just use the first matching node ...
        
            LOG(lgr, 6, "node " << pos->second->m_root);
            found.push_back(pos->second);
        }
    }

    // Now, with the lock no longer held make all of the completion
    // callbacks.
    //
    if (none_found)
    {
        i_func.hnt_error(i_argp, NotFoundError("no starting seed found"));
    }
    else
    {
        for (LameEdgeSeq::const_iterator it = found.begin();
             it != found.end();
             ++it)
            i_func.hnt_node(i_argp, (*it)->m_root);

        i_func.hnt_complete(i_argp);
    }
}

void
LameHeadNodeGraph::insert_head(SignedHeadEdge const & i_she)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_lhnmutex);

    LameEdgeHandle eh = new LameEdge(i_she);

    if (eh->m_prev == eh->m_root)
        throwstream(InternalError, FILELINE
                    << "we'd really rather not have self-loops");

    // Do we already have this edge?
    if (m_prevmap.find(eh->m_prev) != m_prevmap.end() &&
        m_rootmap.find(eh->m_root) != m_rootmap.end())
        return;

    m_prevmap.insert(make_pair(eh->m_prev, eh));

    m_rootmap.insert(make_pair(eh->m_root, eh));

    // Remove any nodes which we preceede from the root set.
    m_roots.erase(eh->m_root);

    // Are we in the root set so far?  If our previous node
    // hasn't been seen we insert ourselves ...
    //
    if (m_rootmap.find(eh->m_prev) == m_rootmap.end())
        m_roots.insert(eh->m_prev);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
