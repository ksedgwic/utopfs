#ifndef UTFS_utfsfwd_h__
#define UTFS_utfsfwd_h__

/// @file utfsfwd.h
/// Forward Declarations for UTFS.

#include "RC.h"

namespace UTFS {

class IndirectBlockNode;
/// Handle to IndirectBlockNode object.
typedef utp::RCPtr<IndirectBlockNode> IndirectBlockNodeHandle;

class DataBlockNode;
/// Handle to DataBlockNode object.
typedef utp::RCPtr<DataBlockNode> DataBlockNodeHandle;

class BlockNode;
/// Handle to BlockNode object.
typedef utp::RCPtr<BlockNode> BlockNodeHandle;

class FileNode;
/// Handle to FileNode object.
typedef utp::RCPtr<FileNode> FileNodeHandle;

class DirNode;
/// Handle to DirNode object.
typedef utp::RCPtr<DirNode> DirNodeHandle;

struct Context;

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_utfsfwd_h__
