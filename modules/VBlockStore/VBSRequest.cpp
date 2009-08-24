#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "vbslog.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSRequest::VBSRequest(VBSRequestHolder & i_vbs, long i_outstanding)
    : m_vbs(i_vbs)
    , m_succeeded(false)
    , m_outstanding(i_outstanding)
{
}

VBSRequest::~VBSRequest()
{
}

bool
VBSRequest::operator<(VBSRequest const & i_o) const
{
    // Just use the address of the request.
    return this < &i_o;
}

void
VBSRequest::done()
{
    // Remove this request from the VBS.
    m_vbs.rh_remove(this);
}

ostream & operator<<(ostream & ostrm, VBSRequest const & i_req)
{
    i_req.stream_insert(ostrm);
    return ostrm;
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
