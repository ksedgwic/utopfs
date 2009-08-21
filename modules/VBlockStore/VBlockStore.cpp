#include "Log.h"
#include "BlockStoreFactory.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "VBSGetRequest.h"
#include "vbslog.h"
#include "VBSPutRequest.h"
#include "VBSRefreshBlockRequest.h"
#include "VBSRefreshFinishRequest.h"
#include "VBSRefreshStartRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

void
VBlockStore::destroy(StringSeq const & i_args)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::destroy unimplemented");
}

VBlockStore::VBlockStore(string const & i_instname)
    : m_instname(i_instname)
    , m_vbscond(m_vbsmutex)
    , m_waiting(false)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");
}

VBlockStore::~VBlockStore()
{
}

string const &
VBlockStore::bs_instname() const
{
    return m_instname;
}

void
VBlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_create unimplemented");
}

void
VBlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, m_instname << ' ' << "bs_open");

    // Insert each of the child blockstores in our collection.
    for (size_t ii = 0; ii < i_args.size(); ++ii)
    {
        string const & instname = i_args[ii];
        m_children.insert(make_pair(instname, new VBSChild(instname)));
    }
}

void
VBlockStore::bs_close()
    throw(InternalError)
{
    // Unregister this instance.
    try
    {
        BlockStoreFactory::unmap(m_instname);
    }
    catch (InternalError const & ex)
    {
        // This we can just rethrow ...
        throw;
    }
    catch (Exception const & ex)
    {
        // These shouldn't happen and need to be converted to
        // InternalError ...
        throw InternalError(ex.what());
    }
}

void
VBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    // For now we presume that the stat call doesn't block and we just
    // call all of the children directly ...

    // Start by setting the values to zero.
    o_stat.bss_size = 0;
    o_stat.bss_free = 0;

    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
    {
        // Make the stat request on the child.
        Stat stat;
        it->second->bs()->bs_stat(stat);

        // Is this bigger then what we have so far?
        if (o_stat.bss_size < stat.bss_size)
        {
            o_stat.bss_size = stat.bss_size;
            o_stat.bss_free = stat.bss_free;
        }
    }
}

void
VBlockStore::bs_sync()
    throw(InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_vbsmutex);

    while (!m_requests.empty())
    {
        m_waiting = true;
        m_vbscond.wait();
    }
}

void
VBlockStore::bs_block_get_async(void const * i_keydata,
                                size_t i_keysize,
                                void * o_buffdata,
                                size_t i_buffsize,
                                BlockGetCompletion & i_cmpl,
                                void const * i_argp)
    throw(InternalError,
          ValueError)
{
    // Create a VBSGetRequest.
    VBSGetRequestHandle grh = new VBSGetRequest(*this,
                                                m_children.size(),
                                                i_keydata,
                                                i_keysize,
                                                o_buffdata,
                                                i_buffsize,
                                                &i_cmpl,
                                                i_argp);

     LOG(lgr, 6, m_instname << ' ' << "bs_block_get_async " << *grh);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsmutex);
        m_requests.insert(grh);
    }

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_get(grh);
}

void
VBlockStore::bs_block_put_async(void const * i_keydata,
                                size_t i_keysize,
                                void const * i_blkdata,
                                size_t i_blksize,
                                BlockPutCompletion & i_cmpl,
                                void const * i_argp)
    throw(InternalError,
          ValueError)
{
    // Create a VBSPutRequest.
    VBSPutRequestHandle prh = new VBSPutRequest(*this,
                                                m_children.size(),
                                                i_keydata,
                                                i_keysize,
                                                i_blkdata,
                                                i_blksize,
                                                &i_cmpl,
                                                i_argp);

    LOG(lgr, 6, m_instname << ' ' << "bs_block_put_async " << *prh);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    insert_request(prh);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_put(prh);
}

void
VBlockStore::bs_refresh_start_async(uint64 i_rid,
                                    RefreshStartCompletion & i_cmpl,
                                    void const * i_argp)
    throw(InternalError)
{
    // Create a request.
    VBSRefreshStartRequestHandle rrh =
        new VBSRefreshStartRequest(*this,
                                   m_children.size(),
                                   i_rid,
                                   i_cmpl,
                                   i_argp);

    LOG(lgr, 6, m_instname << ' ' << "bs_refresh_start_async " << *rrh);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    insert_request(rrh);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_refresh(rrh);
}

void
VBlockStore::bs_refresh_block_async(uint64 i_rid,
                                    void const * i_keydata,
                                    size_t i_keysize,
                                    RefreshBlockCompletion & i_cmpl,
                                    void const * i_argp)
    throw(InternalError,
          NotFoundError)
{
    // Create a request.
    VBSRefreshBlockRequestHandle rrh =
        new VBSRefreshBlockRequest(*this,
                                   m_children.size(),
                                   i_rid,
                                   i_keydata,
                                   i_keysize,
                                   i_cmpl,
                                   i_argp);

    LOG(lgr, 6, m_instname << ' ' << "bs_refresh_block_async " << *rrh);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    insert_request(rrh);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_refresh(rrh);
}
        
void
VBlockStore::bs_refresh_finish_async(uint64 i_rid,
                                     RefreshFinishCompletion & i_cmpl,
                                     void const * i_argp)
    throw(InternalError)
{
    // Create a request.
    VBSRefreshFinishRequestHandle rrh =
        new VBSRefreshFinishRequest(*this,
                                    m_children.size(),
                                    i_rid,
                                    i_cmpl,
                                    i_argp);

    LOG(lgr, 6, m_instname << ' ' << "bs_refresh_finish_async " << *rrh);

    // Insert this request in our request list.  We need to do this
    // first in case the request completes synchrounously below.
    insert_request(rrh);

    // Enqueue the request w/ all of the kids.
    for (VBSChildMap::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
        it->second->enqueue_refresh(rrh);
}

void
VBlockStore::bs_head_insert_async(SignedHeadNode const & i_shn,
                                  SignedHeadInsertCompletion & i_cmpl,
                                  void const * i_argp)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_insert_async unimplemented");
}

void
VBlockStore::bs_head_follow_async(SignedHeadNode const & i_shn,
                                  SignedHeadTraverseFunc & i_func,
                                  void const * i_argp)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_follow_async unimplemented");
}

void
VBlockStore::bs_head_furthest_async(SignedHeadNode const & i_shn,
                                    SignedHeadTraverseFunc & i_func,
                                    void const * i_argp)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_furthest_async unimplemented");
}

void
VBlockStore::insert_request(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, m_instname << ' ' << "insert_request " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_vbsmutex);

    m_requests.insert(i_rh);
}

void
VBlockStore::remove_request(VBSRequestHandle const & i_rh)
{
    LOG(lgr, 6, m_instname << ' ' << "remove_request " << *i_rh);

    ACE_Guard<ACE_Thread_Mutex> guard(m_vbsmutex);

    // Erase this request from the set.
    size_t nrm = m_requests.erase(i_rh);
    if (nrm != 1)
        throwstream(InternalError, FILELINE
                    << "expected to remove one request, removed " << nrm);

    // If we've emptied the request list wake any waiters.
    if (m_requests.empty() && m_waiting)
    {
        m_vbscond.broadcast();
        m_waiting = false;
    }
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
