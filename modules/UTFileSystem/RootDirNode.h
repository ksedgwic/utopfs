#ifndef UTFS_RootDirNode_h__
#define UTFS_RootDirNode_h__

/// @file RootDirNode.h
/// Utopia FileSystem Root Directory Node Object.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "DirNode.h"

namespace UTFS {

class UTFS_EXP RootDirNode : public DirNode
{
public:
    RootDirNode();

    virtual ~RootDirNode();

    virtual FileNodeHandle resolve(std::string const & i_path);

    virtual std::pair<DirNodeHandle, std::string>
        resolve_parent(std::string const & i_path);

    virtual int getattr(struct stat * o_statbuf);

    virtual int readdir(off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

private:
    DirNodeHandle			m_sdh;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_RootDirNode_h__
