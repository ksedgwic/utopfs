#ifndef UTFS_RefBlockNode_h__
#define UTFS_RefBlockNode_h__

/// @file RefBlockNode.h
/// Utopia FileSystem Reference Block Node Object.

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
    // Sequence of index offsets and BlockRefs used for updates.
    typedef std::vector<std::pair<off_t, BlockRef> > BindingSeq;

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

        // Called in post-traversal order on block indexes for with
        // list of updated block digests.  Default implementation
        // persists the block node ...
        //
        virtual void bt_update(Context & i_ctxt,
                               RefBlockNode & i_bn,
                               BindingSeq const & i_bbs);

        int bt_retval() const { return m_retval; }

    protected:
        int				m_retval;
    };

    RefBlockNode() {}

    RefBlockNode(BlockRef const & i_ref) : BlockNode(i_ref) {}

    virtual ~RefBlockNode();

    // Traversal option flags.
    enum TraverseFlags
    {
        RB_DEFAULT		= 0x0,	// Read-only, don't create missing blocks.
        RB_MODIFY		= 0x1	// Create missing blocks.
    };

    // Traverse range calling functor methods.
    virtual bool rb_traverse(Context & i_ctxt,
                             unsigned int i_flags,
                             off_t i_base,			// offset of this block
                             off_t i_rngoff,		// offset of range
                             size_t i_rngsize,		// size of range
                             BlockTraverseFunc & i_trav) = 0;

    // Update references in this node and persist.
    virtual void rb_update(Context & i_ctxt, BindingSeq const & i_bs) = 0;

private:

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_RefBlockNode_h__