#ifndef UTFS_SpecialSockNode_h__
#define UTFS_SpecialSockNode_h__

/// @file SpecialSockNode.h
/// Utopia Special Socket Node Object.
///
/// This object implementst the special files in the ".utopfs" directory.
///
/// See README.txt for inheritance diagram.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "FileNode.h"

namespace UTFS {

class UTFS_EXP SpecialSockNode : public FileNode
{
public:
    SpecialSockNode();

    virtual ~SpecialSockNode();

private:
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_SpecialSockNode_h__
