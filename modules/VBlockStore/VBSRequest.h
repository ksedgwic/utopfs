#ifndef VBSRequest_h__
#define VBSRequest_h__

/// @file VBSRequest.h
/// Virtual BlockStore Request Base Class

#include <iosfwd>

#include <ace/Thread_Mutex.h>

#include "BlockStore.h"
#include "utpfwd.h"

#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore Request Base Class
//
class VBS_EXP VBSRequest : public virtual utp::RCObj
{
public:
    VBSRequest(VBlockStore & i_vbs, long i_outstanding);

    virtual ~VBSRequest();

    virtual bool operator<(VBSRequest const & i_o) const;

    virtual void stream_insert(std::ostream & ostrm) const = 0;

    virtual void done();

protected:
    VBlockStore &						m_vbs;

    ACE_Thread_Mutex					m_vbsreqmutex;
    bool								m_succeeded;
    long								m_outstanding;
};

std::ostream & operator<<(std::ostream & ostrm, VBSRequest const & i_req);

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSRequest_h__
