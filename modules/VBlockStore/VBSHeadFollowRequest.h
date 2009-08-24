#ifndef VBSHeadFollowRequest_h__
#define VBSHeadFollowRequest_h__

/// @file VBSHeadFollowRequest.h
/// Virtual BlockStore HeadFollow Request

#include "VBSRequest.h"
#include "LameHeadNodeGraph.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadFollowRequest
    : public VBSRequest
    , public utp::BlockStore::HeadEdgeTraverseFunc
{
public:
    VBSHeadFollowRequest(VBSRequestHolder & i_vbs,
                         long i_outstanding,
                         utp::HeadNode const & i_hn,
                         utp::BlockStore::HeadEdgeTraverseFunc * i_cmpl,
                         void const * i_argp);

    virtual ~VBSHeadFollowRequest();

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

protected:
    void complete();

private:
    typedef std::set<utp::LameEdgeHandle> LameEdgeSet;

    utp::HeadNode									m_hn;
    utp::BlockStore::HeadEdgeTraverseFunc *			m_cmpl;
    void const *									m_argp;

    LameEdgeSet										m_edges;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFollowRequest_h__
