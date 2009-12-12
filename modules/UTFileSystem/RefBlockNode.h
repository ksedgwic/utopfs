#ifndef UTFS_RefBlockNode_h__
#define UTFS_RefBlockNode_h__

/// @file RefBlockNode.h
/// Utopia FileSystem Reference Block Node Object.
///
/// See README.txt for inheritance diagram.

#include <vector>

#include "utpfwd.h"

#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockRef.h"
#include "BlockNode.h"

namespace UTFS {

// The RefBlockNode is an abstract interface for all objects that
// contain references and hence can particiate in a block traversal.
// The FileNode is a RefBlockNode since it contains the intial direct
// references.  The Indirect block nodes all implement the
// RefBlockNode interface as well.
//
class UTFS_EXP RefBlockNode : public BlockNode
{
public:
    // Block traversal functor base class.
    //
    class UTFS_EXP BlockTraverseFunc
    {
    public:
        BlockTraverseFunc() : m_retval(0) {}

        /// Block visit method called on each block in the traversal.
        // The i_blkoff argument expresses the offset that this block
        // represents in the logical file.  Returns true if the block
        // was modified.
        //
        virtual bool bt_visit(Context & i_ctxt,
                              void * i_blkdata,
                              size_t i_blksize,
                              off_t i_blkoff,
                              size_t i_filesz) = 0;

        int bt_retval() const { return m_retval; }

    protected:
        int				m_retval;
    };

    RefBlockNode() {}

    RefBlockNode(BlockRef const & i_ref) : BlockNode(i_ref) {}

    virtual ~RefBlockNode();

#if 0
    // Traversal Mode.
    //
    // RB_READONLY traversals throw OperationError if the desired
    // block is not already in the dirty tree or clean cache.  A
    // read-lock is all that is required for this access.
    //
    // RB_REFRESH traversals will read missing blocks from the block
    // store but will not insert them in the clean cache to avoid
    // diluting the cache.  A read-lock is all that is required for
    // this traversal.
    //
    // RB_MODIFY traversals will create new blocks, read existing
    // blocks and insert them in the clean cache.  A write-lock
    // is required for RB_MODIFY traversals.
    //
    // [Should these be separate flags instead of an enum?]
    //
    enum TraverseMode
    {
        RB_READONLY		= 0x0,	// read-lock required, cache clean nodes
        RB_REFRESH		= 0x1,	// read-lock required, suppress cache
        RB_MODIFY_X		= 0x2,	// write-lock required, cache clean nodes
    };
#endif

    enum TraverseFlags
    {
        RB_MODIFY_X		= 0x1,	// write-lock required, can dirty nodes
        RB_NOCACHE		= 0x2,	// don't insert in clean cache (refresh)
    };

    // Traverse range calling functor methods.
    virtual bool rb_traverse(Context & i_ctxt,
                             FileNode & i_fn,
                             unsigned i_flags,
                             off_t i_base,			// offset of this block
                             off_t i_rngoff,		// offset of range
                             size_t i_rngsize,		// size of range
                             BlockTraverseFunc & i_trav) = 0;

    // Traverse performing file truncation.  Returns blocks count.
    virtual size_t rb_truncate(Context & i_ctxt,
                               off_t i_base,
                               off_t i_size) = 0;

    // Traverse refreshing all blocks, returns block count.
    virtual size_t rb_refresh(Context & i_ctxt, utp::uint64 i_rid) = 0;

protected:
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_RefBlockNode_h__
