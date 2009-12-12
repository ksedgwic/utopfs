#include "BlockRef.h"
#include "BlockNode.h"

#include "DataBlockNode.h"

#include "BlockNodeCache.h"
#include "utfslog.h"

using namespace std;
using namespace utp;

namespace UTFS {

BlockNodeCache::BlockNodeCache()
{
}

BlockNodeCache::~BlockNodeCache()
{
}

void
BlockNodeCache::insert(BlockNodeHandle const & i_bnh)
{

    BlockRef const & ref = i_bnh->bn_blkref();

    ACE_Guard<ACE_Thread_Mutex> guard(m_bncmutex);

    LOG(lgr, 6, "insert " << ref);

    // Is it already in the cache?
    BlockNodeMap::const_iterator pos = m_nodemap.find(ref);
    if (pos != m_nodemap.end())
    {
        LOG(lgr, 2, "INSERT COLLISION: " << ref);
        LOG(lgr, 2, "EXISTING: " << *(pos->second));
        LOG(lgr, 2, "INSERTED: " << *i_bnh);
        
        throwstream(InternalError, FILELINE
                    << "BlockNode " << ref << " already in cache");
    }

    m_nodemap.insert(make_pair(ref, i_bnh));
    m_nodemru.push_front(i_bnh);
    i_bnh->m_lpos = m_nodemru.begin();
}

BlockNodeHandle
BlockNodeCache::lookup(BlockRef const & i_ref)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_bncmutex);

    BlockNodeMap::const_iterator pos = m_nodemap.find(i_ref);

    // Was this a cache miss?
    if (pos == m_nodemap.end())
        return NULL;

    BlockNodeHandle bnh = pos->second;

    // Move the node to the recent side of the MRU list.
    touch(bnh);

    return bnh;
}

void
BlockNodeCache::remove(BlockRef const & i_ref)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_bncmutex);

    LOG(lgr, 6, "remove " << i_ref);

    // Find the block node.
    BlockNodeMap::const_iterator pos = m_nodemap.find(i_ref);

    // If it's missing we're all done.
    if (pos == m_nodemap.end())
        return;

    // Make a copy of the reference.
    BlockNodeHandle bnh = pos->second;

    // Erase it.
    m_nodemap.erase(pos);

    // Remove the node from the MRU list too.
    m_nodemru.erase(bnh->m_lpos);

    // For good measure set the iterator outside the list.
    bnh->m_lpos = m_nodemru.end();
}

void
BlockNodeCache::get_stats(StatSet & o_ss) const
{
    size_t bncsz;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_bncmutex);
        bncsz = m_nodemap.size();
    }

    Stats::set(o_ss, "bncsz", bncsz, 1.0/1000, "%.1fk", SF_VALUE);
}

void
BlockNodeCache::touch(BlockNodeHandle const & i_bnh)
{
    // IMPORTANT - The caller needs to be holding the mutex!

    // IMPORTANT - The handle better already be in the list!

    m_nodemru.erase(i_bnh->m_lpos);
    m_nodemru.push_front(i_bnh);
    i_bnh->m_lpos = m_nodemru.begin();
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
