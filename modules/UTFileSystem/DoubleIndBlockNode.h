#ifndef UTFS_DoubleIndBlockNode_h__
#define UTFS_DoubleIndBlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Double Indirect Block Node Object.

#include <vector>

#include "utpfwd.h"

#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockRef.h"
#include "IndirectBlockNode.h"

namespace UTFS {

class UTFS_EXP DoubleIndBlockNode : public IndirectBlockNode
{
public:
    // How many digests fit in a packed block.
    static const size_t NUMREF = (BLKSZ / sizeof(BlockRef));

    // Default constructor.
    DoubleIndBlockNode();

    // Constructor from blockstore persisted data.
    DoubleIndBlockNode(Context & i_ctxt, BlockRef const & i_ref);

    virtual ~DoubleIndBlockNode();

    virtual bool rb_traverse(Context & i_ctxt,
                             FileNode & i_fn,
                             unsigned int i_flags,
                             off_t i_base,
                             off_t i_rngoff,
                             size_t i_rngsize,
                             BlockTraverseFunc & i_trav);

    virtual void rb_update(Context & i_ctxt,
                           off_t i_base,
                           BindingSeq const & i_bs);
};

// The ZeroDoubleIndBlockNode is a singleton which is used for
// read-only traversals of a sparse files.  If the read heads in to
// uninitialized territiory we route it through this block ...
//
class UTFS_EXP ZeroDoubleIndBlockNode : public DoubleIndBlockNode
{
public:
    // Constructor from a zero data block.
    ZeroDoubleIndBlockNode(IndirectBlockNodeHandle const & i_nh);

    // This would be a mistake.
    virtual BlockRef bn_persist(Context & i_ctxt);
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DoubleIndBlockNode_h__