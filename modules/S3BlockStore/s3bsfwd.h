#ifndef s3bsfwd_h__
#define s3bsfwd_h__

/// @file s3bsfwd.h

#include <vector>

#include "Log.h"
#include "RC.h"

namespace S3BS {

class S3BlockStore;

class ResponseHandler;
/// Handle to ResponseHandler object.
typedef utp::RCPtr<ResponseHandler> ResponseHandlerHandle;
typedef std::vector<ResponseHandlerHandle> ResponseHandlerSeq;

class PutHandler;
/// Handle to PutHandler object.
typedef utp::RCPtr<PutHandler> PutHandlerHandle;

class GetHandler;
/// Handle to GetHandler object.
typedef utp::RCPtr<GetHandler> GetHandlerHandle;

class ListHandler;
/// Handle to ListHandler object.
typedef utp::RCPtr<ListHandler> ListHandlerHandle;

class AsyncGetHandler;
/// Handle to AsyncGetHandler object.
typedef utp::RCPtr<AsyncGetHandler> AsyncGetHandlerHandle;

class AsyncPutHandler;
/// Handle to AsyncPutHandler object.
typedef utp::RCPtr<AsyncPutHandler> AsyncPutHandlerHandle;

class ObjectDestroyer;
typedef utp::RCPtr<ObjectDestroyer> ObjectDestroyerHandle;

} // end namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // s3bsfwd_h__
