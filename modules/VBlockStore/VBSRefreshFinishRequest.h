#ifndef VBSRefreshFinishRequest_h__
#define VBSRefreshFinishRequest_h__

/// @file VBSRefreshFinishRequest.h
/// Virtual BlockStore RefreshFinish Request

#include "BlockStore.h"

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSRefreshFinishRequest
    : public VBSRequest
    , public utp::BlockStore::RefreshFinishCompletion
{
public:
    VBSRefreshFinishRequest(VBlockStore & i_vbs,
                            long i_outstanding,
                            utp::uint64 i_rid,
                            utp::BlockStore::RefreshFinishCompletion & i_cmpl,
                            void const * i_argp);

    virtual ~VBSRefreshFinishRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // RefreshFinishCompletion

    virtual void rf_complete(utp::uint64 i_rid,
                             void const * i_argp);

    virtual void rf_error(utp::uint64 i_rid,
                          void const * i_argp,
                          utp::Exception const & i_exp);

private:
    utp::uint64									m_rid;
    utp::BlockStore::RefreshFinishCompletion &	m_cmpl;
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
