#ifndef UTFS_BlockNode_h__
#define UTFS_BlockNode_h__

/// @file BlockNode.h
/// Utopia FileSystem Block Node Object.

#include <vector>

#include "utpfwd.h"

#include "Digest.h"
#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

namespace UTFS {

// Base class for block objects.
//
class UTFS_EXP BlockNode : public virtual utp::RCObj
{
public:
    // Default constructor.
    BlockNode() {}

    // Constructor which initializes the cached digest.
    BlockNode(utp::Digest const & i_digest) : m_digest(i_digest) {}

    // Fundamental block size of the filesystem.
    static const size_t BLKSZ = 8192;

    // Destructor.
    virtual ~BlockNode();

    // Returns the cached digest of the node.  This value is not
    // computed until persist() is called.
    //
    virtual utp::Digest const & bn_digest() { return m_digest; }

    // Set's the cached digest of the node.
    //
    virtual void bn_digest(utp::Digest const & i_digest)
    {
        m_digest = i_digest;
    }

    virtual utp::uint8 const * bn_data() const = 0;

    virtual utp::uint8 * bn_data() = 0;

    // This is the size of contained data, not the size of the node
    // itself.  Data blocks return BLKSZ.  INodes have a smaller
    // initial block and return that size.
    //
    virtual size_t bn_size() const = 0;

private:
    utp::Digest			m_digest;
 };

// Data blocks contain the actual logical file data.
//
class UTFS_EXP DataBlockNode : public BlockNode
{
public:
    // Default constructor.
    DataBlockNode();

    // Constructor from blockstore persisted data.
    DataBlockNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~DataBlockNode();

    virtual utp::uint8 const * bn_data() const { return m_data; }

    virtual utp::uint8 * bn_data() { return m_data; }

    virtual size_t bn_size() const { return sizeof(m_data); }

private:
    utp::uint8				m_data[BLKSZ];
};

// The RefBlockNode is an abstract interface for all objects that
// contain references and hence can particiate in a block traversal.
// The FileNode is a RefBlockNode since it contains the intial direct
// references.  The Indirect block nodes all implement the
// RefBlockNode interface as well.
//
class UTFS_EXP RefBlockNode : public BlockNode
{
public:
    // Sequence of index offsets and Digests used for updates.
    typedef std::vector<std::pair<size_t, utp::Digest> > BindingSeq;

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
                              off_t i_blkoff) = 0;

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

    RefBlockNode(utp::Digest const & i_digest) : BlockNode(i_digest) {}

    virtual ~RefBlockNode();

    // Traverse range calling functor methods.
    virtual void rb_traverse(Context & i_ctxt,
                             off_t i_base,			// offset of this block
                             off_t i_rngoff,		// offset of range
                             size_t i_rngsize,		// size of range
                             BlockTraverseFunc & i_trav) = 0;

    // Update references in this node and persist.
    virtual void rb_update(Context & i_ctxt, BindingSeq const & i_bs) = 0;

private:

};

class UTFS_EXP IndirectBlockNode : public BlockNode
{
public:
    // How many digests fit in a packed block.
    static const size_t NUMDIG = (BLKSZ / sizeof(utp::Digest));

    // Default constructor.
    IndirectBlockNode();

    // Constructor from blockstore persisted data.
    IndirectBlockNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~IndirectBlockNode();

    virtual utp::uint8 const * bn_data() const
    {
        return (utp::uint8 const *) &m_reftbl[0];
    }

    virtual utp::uint8 * bn_data() { return (utp::uint8 *) &m_reftbl[0]; }

    // Return the whole block size even if Digests don't divide evenly;
    // gurantees that we initialize all bytes ...
    //
    virtual size_t bn_size() const { return BLKSZ; }

    // Update entries in this node and persist.
    virtual void rb_update(Context & i_ctxt,
                           RefBlockNode::BindingSeq const & i_bs);

private:
    utp::Digest				m_reftbl[NUMDIG];

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_BlockNode_h__
