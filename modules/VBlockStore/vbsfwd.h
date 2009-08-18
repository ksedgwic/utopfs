#ifndef vbsfwd_h__
#define vbsfwd_h__

/// @file vbsfwd.h

#include "Log.h"
#include "RC.h"

namespace VBS {

class VBlockStore;

class VBSChild;
/// Handle to VBSChild object.
typedef utp::RCPtr<VBSChild> VBSChildHandle;

class VBSRequest;
/// Handle to VBSRequest object.
typedef utp::RCPtr<VBSRequest> VBSRequestHandle;

class VBSPutRequest;
/// Handle to VBSPutRequest object.
typedef utp::RCPtr<VBSPutRequest> VBSPutRequestHandle;

} // end namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // vbsfwd_h__
