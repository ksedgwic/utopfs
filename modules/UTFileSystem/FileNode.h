#ifndef UTFS_FileNode_h__
#define UTFS_FileNode_h__

/// @file FileNode.h
/// Utopia FileSystem File Node Object.

#include <string>

#include "Types.h"
#include "Digest.h"

#include "INode.pb.h"

#include "utpfwd.h"
#include "utfsexp.h"


namespace UTFS {

class UTFS_EXP FileNode : public virtual utp::RCObj
{
public:
    FileNode();

    virtual ~FileNode();

    virtual void persist();

    virtual int getattr(struct stat * o_statbuf);

    virtual int read(void * o_bufptr, size_t i_size, off_t i_off);

private:
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
