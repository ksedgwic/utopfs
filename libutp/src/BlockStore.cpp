#include "Except.h"
#include "BlockStore.h"

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>

using namespace utp;

namespace {

class SyncBlockGetCompletion : public BlockStore::BlockGetCompletion
{
public:
    SyncBlockGetCompletion(void * o_outbuff, size_t i_bufsize)
        : m_outbuf(o_outbuff)
        , m_bufsize(i_bufsize)
        , m_except(NULL)
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
                             void const * i_blkdata,
                             size_t i_blksize)
    {
        if (i_blksize > m_bufsize)
        {
            m_except = new ValueError("buffer overflow");
        }
        else
        {
            ACE_OS::memcpy(m_outbuf, i_blkdata, i_blksize);
            m_size = i_blksize;
        }

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

    void rethrow()
    {
        switch (m_except->type())
        {
        case Exception::T_INTERNAL:
            throw InternalError(*m_except);

        case Exception::T_NOTFOUND:
            throw NotFoundError(*m_except);

        case Exception::T_VALUE:
            throw ValueError(*m_except);

        default:
            throw * m_except;
        }
    }

    size_t size() { return m_size; }

private:
    void *						m_outbuf;
    size_t						m_bufsize;
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
    // Make a key sequence w/ just our key in it.
    KeySeq keys;
    keys.push_back(OctetSeq((uint8 const *) i_keydata,
                            (uint8 const *) i_keydata + i_keysize));

    // Create our completion handler.
    SyncBlockGetCompletion sbgc(o_outbuff, i_outsize);

    // Initiate the asynchrounous get.
    bs_get_blocks_async(keys, sbgc);

    // Wait for completion.
    sbgc.wait();

    // If there was an exception, throw it.
    if (sbgc.is_error())
        sbgc.rethrow();

    // Otherwise, return the transferred size.
    return sbgc.size();
}

} // end namespace utp
