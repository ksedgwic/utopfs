#ifndef VBSRefreshStartRequest_h__
#define VBSRefreshStartRequest_h__

/// @file VBSRefreshStartRequest.h
/// Virtual BlockStore RefreshStart Request

#include "BlockStore.h"

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSRefreshStartRequest
    : public VBSRequest
    , public utp::BlockStore::RefreshStartCompletion
{
public:
    VBSRefreshStartRequest(VBSRequestHolder & i_vbs,
                           long i_outstanding,
                           utp::uint64 i_rid,
                           utp::BlockStore::RefreshStartCompletion & i_cmpl,
                           void const * i_argp);

    virtual ~VBSRefreshStartRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // RefreshStartCompletion

    virtual void rs_complete(utp::uint64 i_rid,
                             void const * i_argp);

    virtual void rs_error(utp::uint64 i_rid,
                          void const * i_argp,
                          utp::Exception const & i_exp);

private:
    utp::uint64									m_rid;
    utp::BlockStore::RefreshStartCompletion	&	m_cmpl;
    void const *								m_argp;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSPutRequest_h__
