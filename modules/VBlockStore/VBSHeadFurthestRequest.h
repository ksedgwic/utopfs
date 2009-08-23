#ifndef VBSHeadFurthestRequest_h__
#define VBSHeadFurthestRequest_h__

/// @file VBSHeadFurthestRequest.h
/// Virtual BlockStore HeadFurthest Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadFurthestRequest
    : public VBSRequest
    , public utp::BlockStore::HeadNodeTraverseFunc
{
public:
    VBSHeadFurthestRequest(VBlockStore & i_vbs,
                           long i_outstanding,
                           utp::HeadNode const & i_hn,
                           utp::BlockStore::HeadNodeTraverseFunc * i_cmpl,
                           void const * i_argp);

    virtual ~VBSHeadFurthestRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // SignedHeadTraverseFunc

    virtual void hnt_node(void const * i_argp,
                          utp::HeadNode const & i_hn);

    virtual void hnt_complete(void const * i_argp);

    virtual void hnt_error(void const * i_argp,
                           utp::Exception const & i_exp);

private:
    utp::HeadNode									m_hn;
    utp::BlockStore::HeadNodeTraverseFunc *			m_cmpl;
    void const *									m_argp;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFurthestRequest_h__
