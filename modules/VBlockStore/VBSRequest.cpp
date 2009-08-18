#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "vbslog.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSRequest::VBSRequest(VBlockStore & i_vbs)
    : m_vbs(i_vbs)
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
    m_vbs.remove_request(this);
}

ostream & operator<<(ostream & ostrm, VBSRequest const & i_req)
{
    i_req.stream_insert(ostrm);
    return ostrm;
}

VBSPutRequest::VBSPutRequest(VBlockStore & i_vbs,
                             void const * i_keydata,
                             size_t i_keysize,
                             void const * i_blkdata,
                             size_t i_blksize,
                             BlockStore::BlockPutCompletion & i_cmpl,
                             void const * i_argp)
    : VBSRequest(i_vbs)
    , m_keydata(i_keydata)
    , m_keysize(i_keysize)
    , m_blkdata(i_blkdata)
    , m_blksize(i_blksize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "PUT @" << (void *) this << " CTOR");
}

VBSPutRequest::~VBSPutRequest()
{
    LOG(lgr, 6, "PUT @" << (void *) this << " DTOR");
}

void
VBSPutRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "PUT @" << (void *) this;
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
