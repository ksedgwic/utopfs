#ifndef VBSHeadFollowFillReq_h__
#define VBSHeadFollowFillReq_h__

/// @file VBSHeadFollowFillReq.h
/// Virtual BlockStore HeadFollow Request

#include "VBSRequest.h"
#include "LameHeadNodeGraph.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadFollowFillReq
    : public VBSRequest
    , public utp::BlockStore::HeadEdgeTraverseFunc
    , public utp::BlockStore::HeadEdgeInsertCompletion
{
public:
    VBSHeadFollowFillReq(VBSRequestHolder & i_vbs,
                         long i_outstanding,
                         utp::HeadNode const & i_hn,
                         utp::BlockStore::HeadEdgeTraverseFunc * i_cmpl,
                         void const * i_argp,
                         VBSChild * i_targcp);

    virtual ~VBSHeadFollowFillReq();

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

    // HeadEdgeInsertCompletion

    virtual void hei_complete(utp::SignedHeadEdge const & i_she,
                              void const * i_argp);

    virtual void hei_error(utp::SignedHeadEdge const & i_she,
                           void const * i_argp,
                           utp::Exception const & i_exp);
protected:
    void complete();

private:
    utp::HeadNode									m_hn;
    utp::BlockStore::HeadEdgeTraverseFunc *			m_cmpl;
    void const *									m_argp;
    VBSChild *										m_targcp;
    bool											m_travdone;
    utp::AtomicLong									m_fillsout;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFollowFillReq_h__
