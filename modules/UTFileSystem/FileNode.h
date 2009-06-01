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

class UTFS_EXP FileNode : public RefBlockNode
{
public:
    // How much data is "inlined" in the FileNode itself.
    static const size_t INLSZ = 4096;

    // How many direct references does the FileNode contain?
    static const size_t NDIRECT = 20;

    // Default constructor.
    FileNode();

    // Constructor from blockstore persisted data.
    FileNode(Context & i_ctxt, utp::Digest const & i_dig);

    // Destructor.
    virtual ~FileNode();

    virtual void bn_persist(Context & i_ctxt);

    utp::uint8 const * bn_data() const { return m_inl; }

    utp::uint8 * bn_data() { return m_inl; }

    size_t bn_size() const { return sizeof(m_inl); }

    virtual void rb_traverse(Context & i_ctxt,
                             off_t i_base,
                             off_t i_rngoff,
                             size_t i_rngsize,
                             BlockTraverseFunc & i_trav);

    virtual void rb_update(Context & i_ctxt, BindingSeq const & i_bs);

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
    size_t fixed_field_size() const;

private:
    utp::uint8				m_initvec[8];

    INode					m_inode;

 	utp::uint8				m_inl[INLSZ];

    // References
    utp::Digest				m_dirref[NDIRECT];	// Direct references
    utp::Digest				m_sinref;			// Single indirect refs
    utp::Digest				m_dinref;			// Double indirect refs
    utp::Digest				m_tinref;			// Triple indirect refs
    utp::Digest				m_qinref;			// Quad indirect refs

    // Cached Objects
    DataBlockNodeHandle		m_dirobj[NDIRECT];
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_FileNode_h__
