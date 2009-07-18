#include "Except.h"
#include "BlockStore.h"

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>

using namespace utp;

namespace {

class SyncBlockGetCompletion : public BlockStore::BlockGetCompletion
{
public:
    SyncBlockGetCompletion()
        : m_except(NULL)
        , m_sbgccond(m_sbgcmutex)
        , m_complete(false)
    {}

    ~SyncBlockGetCompletion()
    {
        if (m_except)
            delete m_except;
    }

    virtual void bg_complete(void const * i_keydata,
                             size_t i_keysize,
                             size_t i_blksize)
    {
        m_size = i_blksize;

        ACE_Guard<ACE_Thread_Mutex> guard(m_sbgcmutex);
        m_complete = true;
        m_sbgccond.broadcast();
    }

    virtual void bg_error(void const * i_keydata,
                          size_t i_keysize,
                          Exception const & i_ex)
    {
        m_except = i_ex.clone();

        ACE_Guard<ACE_Thread_Mutex> guard(m_sbgcmutex);
        m_complete = true;
        m_sbgccond.broadcast();
    }

    void wait()
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_sbgcmutex);
        while (!m_complete)
            m_sbgccond.wait();
    }

    bool is_error() { return m_except != NULL; }

    void rethrow() { m_except->rethrow(); }

    size_t size() { return m_size; }

private:
    Exception *					m_except;
    size_t						m_size;
    ACE_Thread_Mutex			m_sbgcmutex;
    ACE_Condition_Thread_Mutex	m_sbgccond;
    bool						m_complete;
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
    SyncBlockGetCompletion sbgc;

    // Initiate the asynchrounous get.
    bs_get_block_async(i_keydata, i_keysize, o_outbuff, i_outsize, sbgc);

    // Wait for completion.
    sbgc.wait();

    // If there was an exception, throw it.
    if (sbgc.is_error())
        sbgc.rethrow();

    // Otherwise, return the transferred size.
    return sbgc.size();
}

} // end namespace utp
