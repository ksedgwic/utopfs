#ifndef utp_LameHeadNodeGraph_h__
#define utp_LameHeadNodeGraph_h__

// NOTE - This class is a short-term crutch which implements
// the HeadNode API by storing the entire graph in memory.
//
// Using it is better then reimplementing it from scratch in each
// BlockStore implementation.
//
// Deprecating it with something which does not rely on having the
// entire graph in memory would be much better ...

#include <string>

#include <ace/Thread_Mutex.h>

#include "BlockStore.h"
#include "Except.h"
#include "HeadEdge.pb.h"
#include "RC.h"
#include "Types.h"
#include "utpexp.h"
#include "utpfwd.h"

namespace utp {

// A Edge represents a connection between two nodes.  They're
// reference counted so they can be held in multiple collections.
//
struct UTP_EXP LameEdge : public RCObj
{
    HeadNode			m_prev;
    HeadNode			m_root;
    SignedHeadEdge		m_she;

    LameEdge(SignedHeadEdge const & i_she);

    bool operator<(LameEdge const & i_o) const;
};

typedef RCPtr<LameEdge> LameEdgeHandle;

// Helpful for debugging.
UTP_EXP
std::ostream & operator<<(std::ostream & ostrm, LameEdge const & i_e);

class UTP_EXP LameHeadNodeGraph
{
public:
    void head_insert_async(SignedHeadEdge const & i_shn,
                           BlockStore::HeadEdgeInsertCompletion & i_cmpl,
                           void const * i_argp)
        throw(InternalError);

    void head_follow_async(HeadNode const & i_hn,
                           BlockStore::HeadEdgeTraverseFunc & i_func,
                           void const * i_argp)
        throw(InternalError);

    void head_furthest_async(HeadNode const & i_hn,
                             BlockStore::HeadNodeTraverseFunc & i_func,
                             void const * i_argp)
        throw(InternalError);

    void insert_head(SignedHeadEdge const & i_she);

private:
    typedef std::multimap<HeadNode, LameEdgeHandle> LameEdgeMap;

    ACE_Thread_Mutex			m_lhnmutex;

    LameEdgeMap					m_prevmap;
    LameEdgeMap					m_rootmap;
    HeadNodeSet					m_roots;		// Roots of the graph
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_LameHeadNodeGraph_h__
