#ifndef UTFS_IndirectBlockNode_h__
#define UTFS_IndirectBlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Indirect Block Node Object.

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
    static const size_t NUMREF = (BLKSZ / sizeof(BlockRef));

    // Default constructor.
    IndirectBlockNode();

    // Constructor from blockstore persisted data.
    IndirectBlockNode(Context & i_ctxt, BlockRef const & i_ref);

    virtual ~IndirectBlockNode();

    virtual BlockRef bn_persist(Context & i_ctxt);

    virtual utp::uint8 const * bn_data() const
    {
        return (utp::uint8 const *) &m_reftbl[0];
    }

    virtual utp::uint8 * bn_data() { return (utp::uint8 *) &m_reftbl[0]; }

    virtual size_t bn_size() const { return BLKSZ; }

    virtual bool rb_traverse(Context & i_ctxt,
                             unsigned int i_flags,
                             off_t i_base,
                             off_t i_rngoff,
                             size_t i_rngsize,
                             BlockTraverseFunc & i_trav);

    virtual void rb_update(Context & i_ctxt, BindingSeq const & i_bs);

private:
    BlockRef				m_reftbl[NUMREF];

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_IndirectBlockNode_h__
