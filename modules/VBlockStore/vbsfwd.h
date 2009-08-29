#ifndef vbsfwd_h__
#define vbsfwd_h__

/// @file vbsfwd.h

#include <vector>

#include "Log.h"
#include "RC.h"

namespace VBS {

class VBlockStore;

class VBSChild;
/// Handle to VBSChild object.
typedef utp::RCPtr<VBSChild> VBSChildHandle;

/// Map of named children.
typedef std::map<std::string, VBSChildHandle> VBSChildMap;

/// Map of children to HeadNode sets.
typedef std::map<VBSChild *, utp::HeadNodeSet> ChildNodeSetMap;

class VBSRequest;
/// Handle to VBSRequest object.
typedef utp::RCPtr<VBSRequest> VBSRequestHandle;

/// A set of unique requests.
typedef std::set<VBSRequestHandle> VBSRequestSet;

class VBSGetRequest;
/// Handle to VBSGetRequest object.
typedef utp::RCPtr<VBSGetRequest> VBSGetRequestHandle;

class VBSPutRequest;
/// Handle to VBSPutRequest object.
typedef utp::RCPtr<VBSPutRequest> VBSPutRequestHandle;

class VBSRefreshStartRequest;
/// Handle to VBSRefreshStartRequest object.
typedef utp::RCPtr<VBSRefreshStartRequest> VBSRefreshStartRequestHandle;

class VBSRefreshBlockRequest;
/// Handle to VBSRefreshBlockRequest object.
typedef utp::RCPtr<VBSRefreshBlockRequest> VBSRefreshBlockRequestHandle;

class VBSRefreshFinishRequest;
/// Handle to VBSRefreshFinishRequest object.
typedef utp::RCPtr<VBSRefreshFinishRequest> VBSRefreshFinishRequestHandle;

class VBSHeadInsertRequest;
/// Handle to VBSHeadInsertRequest object.
typedef utp::RCPtr<VBSHeadInsertRequest> VBSHeadInsertRequestHandle;

class VBSHeadFurthestTopReq;
/// Handle to VBSHeadFurthestTopReq object.
typedef utp::RCPtr<VBSHeadFurthestTopReq> VBSHeadFurthestTopReqHandle;

class VBSHeadFurthestSubReq;
/// Handle to VBSHeadFurthestSubReq object.
typedef utp::RCPtr<VBSHeadFurthestSubReq> VBSHeadFurthestSubReqHandle;

class VBSHeadFollowRequest;
/// Handle to VBSHeadFollowRequest object.
typedef utp::RCPtr<VBSHeadFollowRequest> VBSHeadFollowRequestHandle;

/// Sequence of child handles.
typedef std::vector<VBSChildHandle>	VBSChildSeq;

} // end namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // vbsfwd_h__
