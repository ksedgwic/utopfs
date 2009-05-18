#ifndef UTFS_utfsfwd_h__
#define UTFS_utfsfwd_h__

/// @file utfsfwd.h
/// Forward Declarations for UTFS.

#include "RC.h"

namespace UTFS {

class FileNode;
/// Handle to FileNode object.
typedef utp::RCPtr<FileNode> FileNodeHandle;

class DirNode;
/// Handle to DirNode object.
typedef utp::RCPtr<DirNode> DirNodeHandle;

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_utfsfwd_h__
