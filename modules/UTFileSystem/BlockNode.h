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
    // Fundamental block size of the filesystem.
    static const size_t BLKSZ = 8192;

    // Destructor.
    virtual ~BlockNode();

    // Returns the cached digest of the node.  This value is not
    // computed until persist() is called.
    //
    virtual utp::Digest const & digest() { return m_digest; }

    // Persist the node to the blockstore and update the cached
    // digest value.
    //
    virtual void persist(Context & i_ctxt);

    virtual utp::uint8 const * data() const = 0;

    virtual utp::uint8 * data() = 0;

    virtual size_t size() const = 0;

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

    virtual utp::uint8 const * data() const { return m_data; }

    virtual utp::uint8 * data() { return m_data; }

    virtual size_t size() const { return sizeof(m_data); }

private:
    utp::uint8				m_data[BLKSZ];
};

class UTFS_EXP IndirectBlockNode : public BlockNode
{
public:
    // How many digests fit in a packed block.
    static const size_t NUMDIG = (BLKSZ / sizeof(utp::Digest));

    // Sequence of index offsets and Digests used for updates.
    typedef std::vector<size_t, utp::Digest> BindingSeq;

    // Default constructor.
    IndirectBlockNode();

    // Constructor from blockstore persisted data.
    IndirectBlockNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~IndirectBlockNode();

    virtual utp::uint8 const * data() const
    {
        return (utp::uint8 const *) &m_digest[0];
    }

    virtual utp::uint8 * data() { return (utp::uint8 *) &m_digest[0]; }

    // Return the whole block size even if Digests don't divide evenly;
    // gurantees that we initialize all bytes ...
    //
    virtual size_t size() const { return BLKSZ; }

    // Update entries in this node and persist.
    virtual void update(Context & i_ctxt, BindingSeq const & i_bs);

private:
    utp::Digest				m_digest[NUMDIG];

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_BlockNode_h__
