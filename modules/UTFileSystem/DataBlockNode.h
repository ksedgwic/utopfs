#ifndef UTFS_DataBlockNode_h__
#define UTFS_DataBlockNode_h__

/// @file DataBlockNode.h
/// Utopia FileSystem Block Node Object.

#include <vector>

#include "utpfwd.h"

#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockRef.h"
#include "BlockNode.h"

namespace UTFS {

// Data blocks contain the actual logical file data.
//
class UTFS_EXP DataBlockNode : public BlockNode
{
public:
    // Default constructor.
    DataBlockNode();

    // Constructor from blockstore persisted data.
    DataBlockNode(Context & i_ctxt, BlockRef const & i_ref);

    virtual ~DataBlockNode();

    virtual BlockRef bn_persist(Context & i_ctxt);

    virtual utp::uint8 const * bn_data() const { return m_data; }

    virtual utp::uint8 * bn_data() { return m_data; }

    virtual size_t bn_size() const { return sizeof(m_data); }

private:
    utp::uint8				m_data[BLKSZ];
};

// The ZeroDataBlockNode is a singleton which is used for read-only
// traversals of a sparse files.  If the read heads in to
// uninitialized territiory we route it through this block ...
//
class UTFS_EXP ZeroDataBlockNode : public DataBlockNode
{
public:
    // Default constructor.
    ZeroDataBlockNode();

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

#endif // UTFS_DataBlockNode_h__
