#ifndef UTFS_IndirectBlockNode_h__
#define UTFS_IndirectBlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Indirect Block Node Object.
///
/// See README.txt for inheritance diagram.

#include <vector>

#include "utpfwd.h"

#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockRef.h"
#include "RefBlockNode.h"

namespace UTFS {

class UTFS_EXP IndirectBlockNode : public RefBlockNode
{
public:
    // How many digests fit in a packed block.
    static off_t const NUMREF = (BLKSZ / sizeof(BlockRef));

    // Default constructor.
    IndirectBlockNode();

    // Constructor from blockstore persisted data.
    IndirectBlockNode(Context & i_ctxt, BlockRef const & i_ref);

    virtual ~IndirectBlockNode();

    virtual utp::uint8 const * bn_data() const
    {
        return (utp::uint8 const *) &m_blkref[0];
    }

    virtual utp::uint8 * bn_data() { return (utp::uint8 *) &m_blkref[0]; }

    virtual size_t bn_size() const { return BLKSZ; }

    virtual BlockRef const & bn_persist(Context & i_ctxt);

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
                            
protected:
    // Block References
    BlockRef				m_blkref[NUMREF];

    // Cached Dirty Objects
    BlockNodeHandle			m_blkobj_X[NUMREF];
};

// The ZeroIndirectBlockNode is a singleton which is used for
// read-only traversals of a sparse files.  If the read heads in to
// uninitialized territiory we route it through this block ...
//
class UTFS_EXP ZeroIndirectBlockNode : public IndirectBlockNode
{
public:
    // Constructor from a zero data block.
    ZeroIndirectBlockNode(DataBlockNodeHandle const & i_dbh);

#if 0
    // Debugging, where are we referenced?
	virtual long rc_add_ref(void * ptr = NULL) const;
#endif

    /// Setting us dirty is bad.
    virtual void bn_isdirty(bool i_isdirty);

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

#endif // UTFS_IndirectBlockNode_h__
