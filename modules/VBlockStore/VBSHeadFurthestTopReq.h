#ifndef VBSHeadFurthestTopReq_h__
#define VBSHeadFurthestTopReq_h__

/// @file VBSHeadFurthestTopReq.h
/// Virtual BlockStore HeadFurthest Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadFurthestTopReq
    : public VBSRequest
    , public VBSRequestHolder
    , public utp::BlockStore::HeadNodeTraverseFunc
{
public:
    VBSHeadFurthestTopReq(VBSRequestHolder & i_vbs,
                          long i_outstanding,
                          utp::HeadNode const & i_hn,
                          utp::BlockStore::HeadNodeTraverseFunc * i_cmpl,
                          void const * i_argp,
                          VBSChildMap const & i_children);

    virtual ~VBSHeadFurthestTopReq();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // VBSRequestHolder Methods

    virtual void rh_insert(VBSRequestHandle const & i_rh);

    virtual void rh_remove(VBSRequestHandle const & i_rh);

    // HeadNodeTraverseFunc

    virtual void hnt_node(void const * i_argp,
                          utp::HeadNode const & i_hn);

    virtual void hnt_complete(void const * i_argp);

    virtual void hnt_error(void const * i_argp,
                           utp::Exception const & i_exp);

    // VBSHeadFurthestTopReq

    void init();

protected:

private:
    utp::HeadNode								m_hn;
    utp::BlockStore::HeadNodeTraverseFunc *		m_cmpl;
    void const *								m_argp;

    VBSChildMap const &							m_children;

    ACE_Thread_Mutex							m_hftrmutex;
    VBSRequestSet								m_subreqs;

    VBSHeadFurthestSubReqHandle					m_subfurther;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFurthestTopReq_h__
