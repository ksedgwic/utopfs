#ifndef UTFS_FileNode_h__
#define UTFS_FileNode_h__

/// @file FileNode.h
/// Utopia FileSystem File Node Object.

#include <string>

#include "utpfwd.h"

#include "Types.h"
#include "Digest.h"

#include "INode.pb.h"

#include "utfsexp.h"


namespace UTFS {

class UTFS_EXP FileNode : public virtual utp::RCObj
{
public:
    static const size_t BLKSZ = 4096;

    FileNode();

    FileNode(utp::BlockStoreHandle const & i_bsh,
             utp::StreamCipher & i_cipher,
             utp::Digest const & i_dig);

    virtual ~FileNode();

    virtual utp::Digest const & digest() { return m_digest; }

    virtual void persist(utp::BlockStoreHandle const & i_bsh,
                         utp::StreamCipher & i_cipher);

    virtual int getattr(struct stat * o_statbuf);

    virtual int read(void * o_bufptr, size_t i_size, off_t i_off);

    virtual int write(void const * i_data, size_t i_size, off_t i_off);

private:
    utp::Digest			m_digest;

    utp::uint8			m_initvec[8];

    INode				m_inode;

 	utp::uint8			m_data[2048];

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
