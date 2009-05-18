#ifndef utp_utpfwd_h__
#define utp_utpfwd_h__

/// @file utpfwd.h
/// Forward Declarations.

#include "RC.h"

namespace utp {

class BlockStore;
/// Handle to BlockStore object.
typedef utp::RCPtr<BlockStore> BlockStoreHandle;

class FileSystem;
/// Handle to FileSystem object.
typedef utp::RCPtr<FileSystem> FileSystemHandle;

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_utpfwd_h__
