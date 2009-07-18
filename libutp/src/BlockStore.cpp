#include "Except.h"
#include "BlockStore.h"

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>

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
                             size_t i_blksize)
    {
        m_size = i_blksize;
        done();
    }

    virtual void bg_error(void const * i_keydata,
                          size_t i_keysize,
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
                             size_t i_keysize)
    {
        done();
    }

    virtual void bp_error(void const * i_keydata,
                          size_t i_keysize,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();
        done();
    }
};

class RefreshCompletion
    : public BlockStore::BlockRefreshCompletion
    , public BlockingCompletion
{
public:
    RefreshCompletion(size_t i_count,
                      BlockStore::KeySeq & o_missing)
        : m_count(i_count)
        , m_missing(o_missing)
    {}

    virtual void br_complete(void const * i_keydata,
                             size_t i_keysize)
    {
        if (--m_count == 0)
            done();
    }

    virtual void br_missing(void const * i_keydata,
                            size_t i_keysize)
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

} // end namespace

namespace utp {

BlockStore::~BlockStore()
{
}

size_t
BlockStore::bs_get_block(void const * i_keydata,
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
    bs_get_block_async(i_keydata, i_keysize, o_outbuff, i_outsize, gc);

    // Wait for completion.
    gc.wait();

    // If there was an exception, throw it.
    if (gc.is_error())
        gc.rethrow();

    // Otherwise, return the transferred size.
    return gc.size();
}

void
BlockStore::bs_put_block(void const * i_keydata,
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
    bs_put_block_async(i_keydata, i_keysize, i_blkdata, i_blksize, pc);

    // Wait for completion.
    pc.wait();

    // If there was an exception, throw it.
    if (pc.is_error())
        pc.rethrow();
}

void
BlockStore::bs_refresh_blocks(uint64 i_rid,
                              BlockStore::KeySeq const & i_keys,
                              BlockStore::KeySeq & o_missing)
    throw(InternalError,
          NotFoundError)
{
    // Create our completion handler.
    RefreshCompletion rc(i_keys.size(), o_missing);

    // Initiate all the refreshes asynchronously.
    for (unsigned i = 0; i < i_keys.size(); ++i)
        bs_refresh_block_async(i_rid, &i_keys[i][0], i_keys[i].size(), rc);

    // Wait for completion.
    rc.wait();

    // If there was an exception, throw it.
    if (rc.is_error())
        rc.rethrow();
}

} // end namespace utp
