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
    , public utp::BlockStore::SignedHeadInsertCompletion
{
public:
    VBSHeadInsertRequest(VBlockStore & i_vbs,
                         long i_outstanding,
                         utp::SignedHeadNode const & i_shn,
                         utp::BlockStore::SignedHeadInsertCompletion * i_cmpl,
                         void const * i_argp);

    virtual ~VBSHeadInsertRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void process(VBSChild * i_cp,
                         utp::BlockStoreHandle const & i_bsh);
                         
    // SignedHeadInsertCompletion

    virtual void shi_complete(utp::SignedHeadNode const & i_shn,
                              void const * i_argp);

    virtual void shi_error(utp::SignedHeadNode const & i_shn,
                           void const * i_argp,
                           utp::Exception const & i_exp);

private:
    utp::SignedHeadNode								m_shn;
    utp::BlockStore::SignedHeadInsertCompletion *	m_cmpl;
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
