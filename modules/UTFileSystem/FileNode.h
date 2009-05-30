#ifndef UTFS_FileNode_h__
#define UTFS_FileNode_h__

/// @file FileNode.h
/// Utopia FileSystem File Node Object.

#include <string>

#include "utpfwd.h"

#include "Digest.h"
#include "T64.h"
#include "Types.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

#include "BlockNode.h"

namespace UTFS {

class UTFS_EXP FileNode : public BlockNode
{
public:
    // How much data is "inlined" in the FileNode itself.
    static const size_t INLSZ = 4096;

    // Block traversal functor base class.
    //
    class UTFS_EXP BlockTraverseFunc
    {
    public:
        /// Block visit method called on each block in the traversal.
        // The i_off argument expresses the offset that this block
        // represents in the logical file.
        virtual void bt_visit(Context & i_ctxt,
                              void * i_data,
                              size_t i_size,
                              off_t i_off);

        // Called in post-traversal order on block indexes for with
        // list of updated block digests.  Default implementation
        // persists the block node ...
        //
        virtual void bt_update(Context & i_ctxt,
                               BlockNode & i_bn,
                               IndirectBlockNode::BindingSeq const & i_bbs);
    };

    // Default constructor.
    FileNode();

    // Constructor from blockstore persisted data.
    FileNode(Context & i_ctxt, utp::Digest const & i_dig);

    // Destructor.
    virtual ~FileNode();

    // Persist the node to the blockstore and update the cached
    // digest value.
    //
    virtual void persist(Context & i_ctxt);

    virtual int getattr(Context & i_ctxt,
                        struct stat * o_statbuf);

    virtual int read(Context & i_ctxt,
                     void * o_bufptr,
                     size_t i_size,
                     off_t i_off);

    virtual int write(Context & i_ctxt,
                      void const * i_data,
                      size_t i_size,
                      off_t i_off);

    mode_t mode() const { return m_inode.mode(); }

    void mode(mode_t i_mode) { m_inode.set_mode(i_mode); }

    nlink_t nlink() const { return m_inode.nlink(); }

    void nlink(nlink_t i_nlink) { m_inode.set_nlink(i_nlink); }

    size_t size() const { return m_inode.size(); }

    void size(size_t i_size) { m_inode.set_size(i_size); }

    utp::T64 atime() const { return m_inode.atime(); }

    void atime(utp::T64 const & i_atime) { m_inode.set_atime(i_atime.usec()); }

    utp::T64 mtime() const { return m_inode.mtime(); }

    void mtime(utp::T64 const & i_mtime) { m_inode.set_mtime(i_mtime.usec()); }

protected:
    utp::uint8 const * bn_data() const { return m_inl; }

    utp::uint8 * bn_data() { return m_inl; }

    size_t bn_size() const { return sizeof(m_inl); }

    size_t fixed_field_size() const;

private:
    utp::uint8			m_initvec[8];

    INode				m_inode;

 	utp::uint8			m_inl[INLSZ];

    utp::Digest			m_direct[20];
    utp::Digest			m_sindir;		// single indirect
    utp::Digest			m_dindir;		// double indirect
    utp::Digest			m_tindir;		// triple indirect
    utp::Digest			m_qindir;		// quad indirect
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_FileNode_h__
