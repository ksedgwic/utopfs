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

namespace UTFS {

class UTFS_EXP FileNode : public virtual utp::RCObj
{
public:
    static const size_t BLKSZ = 8192;
    static const size_t INLSZ = 4096;

    FileNode();

    FileNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~FileNode();

    virtual utp::Digest const & digest() { return m_digest; }

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

    utp::uint8 const * data() const { return m_data; }

    utp::uint8 * data() { return m_data; }

    size_t size() { return sizeof(m_data); }

    size_t fixed_field_size() const;

private:
    utp::Digest			m_digest;

    utp::uint8			m_initvec[8];

    INode				m_inode;

 	utp::uint8			m_data[INLSZ];

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
