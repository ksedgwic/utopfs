#ifndef UTFS_SymlinkNode_h__
#define UTFS_SymlinkNode_h__

/// @file SymlinkNode.h
/// Utopia FileSystem Symlink Node Object.
///
/// This object implements symlinks.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "FileNode.h"

namespace UTFS {

class UTFS_EXP SymlinkNode : public FileNode
{
public:
    SymlinkNode(std::string const & i_path);

    // "Upgrade Constructor" from FileNode.
    SymlinkNode(FileNode const & i_fn);

    virtual ~SymlinkNode();

    virtual int readlink(Context & i_ctxt,
                         char * o_obuf,
                         size_t i_size);

private:
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_SymlinkNode_h__
