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
    , public utp::BlockStore::HeadNodeTraverseFunc
    , public utp::BlockStore::HeadEdgeTraverseFunc
    
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

    // HeadEdgeTraverseFunc

    virtual void het_edge(void const * i_argp,
                          utp::SignedHeadEdge const & i_she);

    virtual void het_complete(void const * i_argp);

    virtual void het_error(void const * i_argp,
                           utp::Exception const & i_exp);

    // HeadNodeTraverseFunc

    virtual void hnt_node(void const * i_argp,
                          utp::HeadNode const & i_hn);

    virtual void hnt_complete(void const * i_argp);

    virtual void hnt_error(void const * i_argp,
                           utp::Exception const & i_exp);

    // VBSHeadFurthestTopReq

    // Initiates a furthest pass to see if there are unique answers.
    void init();

    // Initiates a directed follow-fill to fast-forward unique answers.
    void directed_follow_fill();

    // Initiates a second furthest pass to see if follow-fill worked.
    void second_check();

    // Initiates a full follow-fill to copy everything.
    void full_follow_fill();

    // Initiates final furthest pass to see of full-follow-fill worked.
    void last_check();

protected:

private:
    utp::HeadNode								m_hn;
    utp::BlockStore::HeadNodeTraverseFunc *		m_cmpl;
    void const *								m_argp;

    VBSChildMap const &							m_children;

    VBSHeadFurthestSubReqHandle					m_subfurther;

    bool										m_lasttry;
    utp::AtomicLong								m_ffout;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFurthestTopReq_h__
