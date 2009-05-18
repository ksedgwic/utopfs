#ifndef UTFS_FileNode_h__
#define UTFS_FileNode_h__

/// @file FileNode.h
/// Utopia FileSystem File Node Object.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"


namespace UTFS {

class UTFS_EXP FileNode : public virtual utp::RCObj
{
public:
    FileNode();

    virtual ~FileNode();

    virtual int getattr(struct stat * o_statbuf);

    virtual int read(void * o_bufptr, size_t i_size, off_t i_off);

private:
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_FileNode_h__
