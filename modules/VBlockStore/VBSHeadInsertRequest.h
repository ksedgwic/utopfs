#ifndef VBSHeadInsertRequest_h__
#define VBSHeadInsertRequest_h__

/// @file VBSHeadInsertRequest.h
/// Virtual BlockStore HeadInsert Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSHeadInsertRequest
    : public VBSRequest
    , public utp::BlockStore::HeadEdgeInsertCompletion
{
public:
    VBSHeadInsertRequest(VBSRequestHolder & i_vbs,
                         long i_outstanding,
                         utp::SignedHeadEdge const & i_she,
                         utp::BlockStore::HeadEdgeInsertCompletion * i_cmpl,
                         void const * i_argp);

    virtual ~VBSHeadInsertRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void initiate(VBSChild * i_cp,
                          utp::BlockStoreHandle const & i_bsh);
                         
    // SignedHeadInsertCompletion

    virtual void hei_complete(utp::SignedHeadEdge const & i_she,
                              void const * i_argp);

    virtual void hei_error(utp::SignedHeadEdge const & i_she,
                           void const * i_argp,
                           utp::Exception const & i_exp);

private:
    utp::SignedHeadEdge								m_she;
    utp::BlockStore::HeadEdgeInsertCompletion *		m_cmpl;
    void const *									m_argp;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSHeadInsertRequest_h__
