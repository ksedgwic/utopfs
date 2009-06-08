#ifndef UTFS_BlockNode_h__
#define UTFS_BlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Block Node Object.

#include <vector>

#include "utpfwd.h"

#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockRef.h"

namespace UTFS {

// Base class for block objects.
//
class UTFS_EXP BlockNode : public virtual utp::RCObj
{
public:
    // Fundamental block size of the filesystem.
    static off_t const BLKSZ = 8192;

    // Default constructor.
    BlockNode() {}

    // Constructor which initializes the cached digest.
    BlockNode(BlockRef const & i_ref);

    // Destructor.
    virtual ~BlockNode();

    // Persist the node to the blockstore and update the cached
    // reference.
    //
    virtual BlockRef bn_persist(Context & i_ctxt) = 0;

    // Returns the cached reference of the node.  This value is not
    // computed until bn_persist() is called.
    //
    virtual BlockRef const & bn_blkref() { return m_ref; }

    virtual utp::uint8 const * bn_data() const = 0;

    virtual utp::uint8 * bn_data() = 0;

    // This is the size of contained data, not the size of the node
    // itself.  Data blocks return BLKSZ.  INodes have a smaller
    // initial block and return that size.
    //
    virtual size_t bn_size() const = 0;

protected:
    BlockRef			m_ref;
 };

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_BlockNode_h__
