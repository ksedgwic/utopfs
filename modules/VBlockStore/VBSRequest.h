#ifndef VBSRequest_h__
#define VBSRequest_h__

/// @file VBSRequest.h
/// Virtual BlockStore Request Base Class

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore Request Base Class
//
class VBS_EXP VBSRequest : public utp::RCObj
{
public:
    VBSRequest();

    virtual ~VBSRequest();

protected:

private:
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSRequest_h__
