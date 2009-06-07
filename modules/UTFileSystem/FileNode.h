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

#include "RefBlockNode.h"
#include "DataBlockNode.h"
#include "IndirectBlockNode.h"
#include "DoubleIndBlockNode.h"
#include "BlockRef.h"

namespace UTFS {

class UTFS_EXP FileNode : public RefBlockNode
{
public:
    // How much data is "inlined" in the FileNode itself.
    static const size_t INLSZ = 4096;

    // How many direct references does the FileNode contain?
    static const size_t NDIRECT = 20;

    // Default constructor.
    FileNode(mode_t i_mode);

    // Constructor from blockstore persisted data.
    FileNode(Context & i_ctxt, BlockRef const & i_ref);

    // Destructor.
    virtual ~FileNode();

    virtual BlockRef bn_persist(Context & i_ctxt);

    utp::uint8 const * bn_data() const { return m_inl; }

    utp::uint8 * bn_data() { return m_inl; }

    size_t bn_size() const { return sizeof(m_inl); }

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

    virtual int getattr(Context & i_ctxt,
                        struct stat * o_statbuf);

    virtual int readlink(Context & i_ctxt,
                         char * o_obuf,
                         size_t i_size);

    virtual int chmod(Context & i_ctxt,
                      mode_t i_mode);

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

    size_t blocks() const { return m_inode.blocks(); }

    void blocks(size_t i_blocks) { m_inode.set_blocks(i_blocks); }

protected:
    size_t fixed_field_size() const;

private:
    INode						m_inode;

 	utp::uint8					m_inl[INLSZ];

    // Block References
    BlockRef					m_dirref[NDIRECT];	// Direct references
    BlockRef					m_sinref;			// Single indirect refs
    BlockRef					m_dinref;			// Double indirect refs
    BlockRef					m_tinref;			// Triple indirect refs
    BlockRef					m_qinref;			// Quad indirect refs

    // Cached Objects
    DataBlockNodeHandle			m_dirobj[NDIRECT];
    IndirectBlockNodeHandle		m_sinobj;
    DoubleIndBlockNodeHandle	m_dinobj;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_FileNode_h__
