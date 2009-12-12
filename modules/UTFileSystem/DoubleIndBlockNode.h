#ifndef UTFS_DoubleIndBlockNode_h__
#define UTFS_DoubleIndBlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Double Indirect Block Node Object.
///
/// See README.txt for inheritance diagram.

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
    static off_t const NUMREF = (BLKSZ / sizeof(BlockRef));

    // Default constructor.
    DoubleIndBlockNode();

    // Constructor from blockstore persisted data.
    DoubleIndBlockNode(Context & i_ctxt, BlockRef const & i_ref);

    virtual ~DoubleIndBlockNode();

    virtual BlockRef const & bn_flush(Context & i_ctxt);

    virtual void bn_tostream(std::ostream & ostrm) const;

    virtual bool rb_traverse(Context & i_ctxt,
                             FileNode & i_fn,
                             unsigned i_flags,
                             off_t i_base,
                             off_t i_rngoff,
                             size_t i_rngsize,
                             BlockTraverseFunc & i_trav);

    virtual size_t rb_truncate(Context & i_ctxt,
                               off_t i_base,
                               off_t i_size);

    virtual size_t rb_refresh(Context & i_ctxt, utp::uint64 i_rid);
                            
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
    virtual BlockRef const & bn_persist(Context & i_ctxt);

    virtual void bn_tostream(std::ostream & ostrm) const;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DoubleIndBlockNode_h__
