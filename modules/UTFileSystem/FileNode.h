#ifndef UTFS_FileNode_h__
#define UTFS_FileNode_h__

/// @file FileNode.h
/// Utopia FileSystem File Node Object.

#include <string>

#include "utpfwd.h"

#include "Types.h"
#include "Digest.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "INode.pb.h"

namespace UTFS {

class UTFS_EXP FileNode : public virtual utp::RCObj
{
public:
    static const size_t BLKSZ = 4096;

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
