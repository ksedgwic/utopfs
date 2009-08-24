#ifndef VBSRefreshBlockRequest_h__
#define VBSRefreshBlockRequest_h__

/// @file VBSRefreshBlockRequest.h
/// Virtual BlockStore RefreshBlock Request

#include "BlockStore.h"

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSRefreshBlockRequest
    : public VBSRequest
    , public utp::BlockStore::RefreshBlockCompletion
{
public:
    VBSRefreshBlockRequest(VBSRequestHolder & i_vbs,
                           long i_outstanding,
                           utp::uint64 i_rid,
                           void const * i_keydata,
                           size_t i_keysize,
                           utp::BlockStore::RefreshBlockCompletion & i_cmpl,
                           void const * i_argp);

    virtual ~VBSRefreshBlockRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // RefreshBlockCompletion

    virtual void rb_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp);

    virtual void rb_missing(void const * i_keydata,
                            size_t i_keysize,
                            void const * i_argp);

private:
    utp::uint64									m_rid;
    utp::OctetSeq								m_key;
    utp::BlockStore::RefreshBlockCompletion	&	m_cmpl;
    void const *								m_argp;
    VBSChildSeq									m_needy;
    VBSChildHandle								m_hadit;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSPutRequest_h__
