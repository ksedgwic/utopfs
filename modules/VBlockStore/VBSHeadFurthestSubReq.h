#ifndef VBSHeadFurthestSubReq_h__
#define VBSHeadFurthestSubReq_h__

/// @file VBSHeadFurthestSubReq.h
/// Virtual BlockStore HeadFurthest Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadFurthestSubReq
    : public VBSRequest
    , public utp::BlockStore::HeadNodeTraverseFunc
{
public:
    VBSHeadFurthestSubReq(VBlockStore & i_vbs,
                          long i_outstanding,
                          utp::HeadNode const & i_hn,
                          utp::BlockStore::HeadNodeTraverseFunc * i_cmpl,
                          void const * i_argp);

    virtual ~VBSHeadFurthestSubReq();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // HeadNodeTraverseFunc

    virtual void hnt_node(void const * i_argp,
                          utp::HeadNode const & i_hn);

    virtual void hnt_complete(void const * i_argp);

    virtual void hnt_error(void const * i_argp,
                           utp::Exception const & i_exp);

    // VBSHeadFurthestSubReq

    // All of the nodes that all clients have in common.
    utp::HeadNodeSeq const & common() const { return m_cmnnodes; }

    // Any unique nodes that children have.
    ChildNodeSetMap const & unique() const { return m_unqnodes; }

protected:
    void complete();

private:
    utp::HeadNode								m_hn;
    utp::BlockStore::HeadNodeTraverseFunc *		m_cmpl;
    void const *								m_argp;

    ChildNodeSetMap								m_cnsm;
    utp::HeadNodeSeq							m_cmnnodes;
    ChildNodeSetMap								m_unqnodes;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadFurthestSubReq_h__
