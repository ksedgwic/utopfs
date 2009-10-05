#include <vector>

#include "Base32.h"
#include "Except.h"
#include "BlockStore.h"

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>

using namespace std;
using namespace utp;

namespace {

class BlockingCompletion
{
public:
    BlockingCompletion()
        : m_except(NULL)
        , m_bccond(m_bcmutex)
        , m_complete(false)
    {}

    ~BlockingCompletion()
    {
        if (m_except)
            delete m_except;
    }

    void done()
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_bcmutex);
        m_complete = true;
        m_bccond.broadcast();
    }
        
    void wait()
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_bcmutex);
        while (!m_complete)
            m_bccond.wait();
    }

    bool is_error() { return m_except != NULL; }

    void rethrow() { m_except->rethrow(); }

protected:
    Exception *					m_except;
    ACE_Thread_Mutex			m_bcmutex;
    ACE_Condition_Thread_Mutex	m_bccond;
    bool						m_complete;
};

class GetCompletion
    : public BlockStore::BlockGetCompletion
    , public BlockingCompletion
{
public:
    virtual void bg_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp,
                             size_t i_blksize)
    {
        m_size = i_blksize;
        done();
    }

    virtual void bg_error(void const * i_keydata,
                          size_t i_keysize,
                          void const * i_argp,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();
        done();
    }

    size_t size() { return m_size; }

private:
    size_t						m_size;
};

class PutCompletion
    : public BlockStore::BlockPutCompletion
    , public BlockingCompletion
{
public:
    virtual void bp_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp)
    {
        done();
    }

    virtual void bp_error(void const * i_keydata,
                          size_t i_keysize,
                          void const * i_argp,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();
        done();
    }
};

class BSRefreshStartCompletion
    : public BlockStore::RefreshStartCompletion
    , public BlockingCompletion
{
public:
    BSRefreshStartCompletion()
    {}

    virtual void rs_complete(uint64 i_rid,
                             void const * i_argp)
    {
        done();
    }

    virtual void rs_error(uint64 i_rid,
                          void const * i_argp,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();
        done();
    }

private:
};

class BSRefreshBlocksCompletion
    : public BlockStore::RefreshBlockCompletion
    , public BlockingCompletion
{
public:
    BSRefreshBlocksCompletion(size_t i_count,
                              BlockStore::KeySeq & o_missing)
        : m_count(i_count)
        , m_missing(o_missing)
    {}

    virtual void rb_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp)
    {
        if (--m_count == 0)
            done();
    }

    virtual void rb_missing(void const * i_keydata,
                            size_t i_keysize,
                            void const * i_argp)
    {
        m_missing.push_back(OctetSeq((uint8 const *) i_keydata,
                                     (uint8 const *) i_keydata + i_keysize));
        if (--m_count == 0)
            done();
    }

private:
    size_t						m_count;
    BlockStore::KeySeq &		m_missing;
};

class BSRefreshFinishCompletion
    : public BlockStore::RefreshFinishCompletion
    , public BlockingCompletion
{
public:
    BSRefreshFinishCompletion()
    {}

    virtual void rf_complete(uint64 i_rid,
                             void const * i_argp)
    {
        done();
    }

    virtual void rf_error(uint64 i_rid,
                          void const * i_argp,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();
        done();
    }

private:
};

class InsertCompletion
    : public BlockStore::HeadEdgeInsertCompletion
    , public BlockingCompletion
{
public:
    virtual void hei_complete(SignedHeadEdge const & i_she,
                              void const * i_argp)
    {
        done();
    }

    virtual void hei_error(SignedHeadEdge const & i_she,
                           void const * i_argp,
                           Exception const & i_exp)
    {
        m_except = i_exp.clone();
        done();
    }
};

class EdgeTraversalCompletion
    : public BlockStore::HeadEdgeTraverseFunc
    , public BlockingCompletion
{
public:
    EdgeTraversalCompletion(BlockStore::SignedHeadEdgeSeq & o_edges)
        : m_edges(o_edges)
    {}

    virtual void het_edge(void const * i_argp,
                          SignedHeadEdge const & i_she)
    {
        m_edges.push_back(i_she);
    }

    virtual void het_complete(void const * i_argp)
    {
        done();
    }

    virtual void het_error(void const * i_argp,
                           Exception const & i_exp)
    {
        m_except = i_exp.clone();
        done();
    }

private:
    BlockStore::SignedHeadEdgeSeq &		m_edges;
};

class NodeTraversalCompletion
    : public BlockStore::HeadNodeTraverseFunc
    , public BlockingCompletion
{
public:
    NodeTraversalCompletion(HeadNodeSeq & o_nodes)
        : m_nodes(o_nodes)
    {}

    virtual void hnt_node(void const * i_argp,
                          HeadNode const & i_hn)
    {
        m_nodes.push_back(i_hn);
    }

    virtual void hnt_complete(void const * i_argp)
    {
        done();
    }

    virtual void hnt_error(void const * i_argp,
                           Exception const & i_exp)
    {
        m_except = i_exp.clone();
        done();
    }

private:
    HeadNodeSeq &		m_nodes;
};

} // end namespace

namespace utp {

ostream &
operator<<(ostream & ostrm, HeadNode const & i_hn)
{
    ostrm << mkstring(i_hn);
    return ostrm;
}

string keystr(utp::OctetSeq const & i_key)
{
    return keystr(&i_key[0], i_key.size());
}

string keystr(void const * i_data, size_t i_size)
{
    return Base32::encode(i_data, i_size).substr(0, 8) + "...";
}

string mkstring(HeadNode const & i_hn)
{
    string pt1 = Base32::encode(i_hn.first.data(), i_hn.first.size());
    string pt2 = Base32::encode(i_hn.second.data(), i_hn.second.size());

    // Strip any trailing "====" off ...
    pt1 = pt1.substr(0, pt1.find_first_of('='));
    pt2 = pt2.substr(0, pt2.find_first_of('='));

    string::size_type sz1 = pt1.size();
    string::size_type sz2 = pt2.size();

    // How many characters of the FSID and NODEID should we display?
    static string::size_type const NFSID = 3;
    static string::size_type const NNDID = 5;

    // Use the right-justified substrings since the tests sometimes
    // only differ in the right positions.  Real digest based refs
    // will differ in all positions.
    //
    string::size_type off1 = sz1 > NFSID ? sz1 - NFSID : 0;
    string::size_type off2 = sz2 > NNDID ? sz2 - NNDID : 0;

    return pt1.substr(off1) + ':' + pt2.substr(off2);
}

BlockStore::~BlockStore()
{
}

size_t
BlockStore::bs_block_get(void const * i_keydata,
                         size_t i_keysize,
                         void * o_outbuff,
                         size_t i_outsize)
        throw(InternalError,
              NotFoundError,
              ValueError)
{
    // Create our completion handler.
    GetCompletion gc;

    // Initiate the asynchrounous get.
     bs_block_get_async(i_keydata, i_keysize, o_outbuff, i_outsize, gc, NULL);

    // Wait for completion.
    gc.wait();

    // If there was an exception, throw it.
    if (gc.is_error())
        gc.rethrow();

    // Otherwise, return the transferred size.
    return gc.size();
}

void
BlockStore::bs_block_put(void const * i_keydata,
                         size_t i_keysize,
                         void const * i_blkdata,
                         size_t i_blksize)
        throw(InternalError,
              ValueError,
              NoSpaceError)
{
    // Create our completion handler.
    PutCompletion pc;

    // Initiate the asynchrounous get.
    bs_block_put_async(i_keydata, i_keysize, i_blkdata, i_blksize, pc, NULL);

    // Wait for completion.
    pc.wait();

    // If there was an exception, throw it.
    if (pc.is_error())
        pc.rethrow();
}

void
BlockStore::bs_refresh_start(uint64 i_rid)
        throw(InternalError,
              NotUniqueError)
{
    // Create our completion handler.
    BSRefreshStartCompletion rsc;

    // Initiate the asynchrounous refresh start
    bs_refresh_start_async(i_rid, rsc, NULL);

    // Wait for completion.
    rsc.wait();

    // If there was an exception, throw it.
    if (rsc.is_error())
        rsc.rethrow();
}

void
BlockStore::bs_refresh_blocks(uint64 i_rid,
                              BlockStore::KeySeq const & i_keys,
                              BlockStore::KeySeq & o_missing)
    throw(InternalError,
          NotFoundError)
{
    // Create our completion handler.
    BSRefreshBlocksCompletion rbc(i_keys.size(), o_missing);

    // Initiate all the refreshes asynchronously.
    for (unsigned i = 0; i < i_keys.size(); ++i)
        bs_refresh_block_async(i_rid, &i_keys[i][0],
                               i_keys[i].size(), rbc, NULL);

    // Wait for completion.
    rbc.wait();

    // If there was an exception, throw it.
    if (rbc.is_error())
        rbc.rethrow();
}

void
BlockStore::bs_refresh_finish(uint64 i_rid)
        throw(InternalError,
              NotFoundError)
{
    // Create our completion handler.
    BSRefreshFinishCompletion rfc;

    // Initiate the asynchrounous refresh finish
    bs_refresh_finish_async(i_rid, rfc, NULL);

    // Wait for completion.
    rfc.wait();

    // If there was an exception, throw it.
    if (rfc.is_error())
        rfc.rethrow();
}

void
BlockStore::bs_head_insert(SignedHeadEdge const & i_she)
    throw(InternalError)
{
    // Create our completion handler.
    InsertCompletion ic;

    // Initiate the asynchronous insert.
    bs_head_insert_async(i_she, ic, NULL);

    // Wait for completion.
    ic.wait();

    // If there was an exception, throw it.
    if (ic.is_error())
        ic.rethrow();
}

void
BlockStore::bs_head_follow(HeadNode const & i_hn,
                           SignedHeadEdgeSeq & o_edges)
    throw(InternalError,
          NotFoundError)
{
    // Create our completion handler.
    EdgeTraversalCompletion etc(o_edges);

    // Initiate the asynchronous insert.
    bs_head_follow_async(i_hn, etc, NULL);

    // Wait for completion.
    etc.wait();

    // If there was an exception, throw it.
    if (etc.is_error())
        etc.rethrow();
}

void
BlockStore::bs_head_furthest(HeadNode const & i_hn,
                             HeadNodeSeq & o_nodes)
    throw(InternalError,
          NotFoundError)
{
    // Create our completion handler.
    NodeTraversalCompletion ntc(o_nodes);

    // Initiate the asynchronous insert.
    bs_head_furthest_async(i_hn, ntc, NULL);

    // Wait for completion.
    ntc.wait();

    // If there was an exception, throw it.
    if (ntc.is_error())
        ntc.rethrow();
}

} // end namespace utp
