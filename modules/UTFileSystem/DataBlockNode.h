#ifndef UTFS_DataBlockNode_h__
#define UTFS_DataBlockNode_h__

/// @file DataBlockNode.h
/// Utopia FileSystem Block Node Object.
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

    virtual utp::uint8 const * bn_data() const { return m_data; }

    virtual utp::uint8 * bn_data() { return m_data; }

    virtual size_t bn_size() const { return sizeof(m_data); }

    virtual BlockRef const & bn_persist(Context & i_ctxt);

    virtual BlockRef const & bn_flush(Context & i_ctxt);

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

#if 0
    // Debugging, where are we referenced?
	virtual long rc_add_ref(void * ptr = NULL) const;
#endif

    /// Setting us dirty is bad.
    virtual void bn_isdirty(bool i_isdirty);

    // This would be a mistake.
    virtual BlockRef const & bn_persist(Context & i_ctxt);
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DataBlockNode_h__
