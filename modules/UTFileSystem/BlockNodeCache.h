#ifndef UTFS_BlockNodeCache_h__
#define UTFS_BlockNodeCache_h__

/// @file BlockNodeCache.h
/// Utopia FileSystem BlockNode Cache.

#include <tr1/unordered_map>

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "Types.h"

#include "BlockRef.h"
#include "utfsfwd.h"
#include "utfsexp.h"

namespace UTFS {

// BlockNode Cache
//
class UTFS_EXP BlockNodeCache
{
public:
    // Default constructor.
    BlockNodeCache();

    // Destructor.
    ~BlockNodeCache();

    // Insert Node.
    void insert(BlockNodeHandle const & i_bnh);

    // Lookup Node.
    BlockNodeHandle lookup(BlockRef const & i_ref);

    // Remove Node.
    void remove(BlockRef const & i_ref);

protected:
    // Move the node the front (recent) end of the MRU list.
    void touch(BlockNodeHandle const & i_bnh);

private:
    typedef std::tr1::unordered_map<BlockRef,
        							BlockNodeHandle,
        							BlockRef::hash> BlockNodeMap;

    mutable ACE_Thread_Mutex	m_bncmutex;
    BlockNodeMap				m_nodemap;
    BlockNodeList				m_nodemru;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_BlockNodeCache_h__
