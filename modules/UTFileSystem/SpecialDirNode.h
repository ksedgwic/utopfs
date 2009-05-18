#ifndef UTFS_SpecialDirNode_h__
#define UTFS_SpecialDirNode_h__

/// @file SpecialDirNode.h
/// Utopia FileSystem Special Directory Node Object.
///
/// This object implements the special ".utopfs" directory.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "DirNode.h"

namespace UTFS {

class UTFS_EXP SpecialDirNode : public DirNode
{
public:
    SpecialDirNode();

    virtual ~SpecialDirNode();

    virtual FileNodeHandle resolve(std::string const & i_path);

    virtual std::pair<DirNodeHandle, std::string>
        resolve_parent(std::string const & i_path);

    virtual int getattr(struct stat * o_statbuf);

    virtual int open(std::string const & i_entry, int i_flags);

    virtual int readdir(off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

private:
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_SpecialDirNode_h__
