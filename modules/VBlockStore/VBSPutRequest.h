#ifndef VBSPutRequest_h__
#define VBSPutRequest_h__

/// @file VBSPutRequest.h
/// Virtual BlockStore Put Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSPutRequest
    : public VBSRequest
    , public utp::BlockStore::BlockPutCompletion
{
public:
    VBSPutRequest(VBlockStore & i_vbs,
                  long i_outstanding,
                  void const * i_keydata,
                  size_t i_keysize,
                  void const * i_blkdata,
                  size_t i_blksize,
                  utp::BlockStore::BlockPutCompletion * i_cmpl,
                  void const * i_argp);

    virtual ~VBSPutRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    // BlockPutCompletion

    virtual void bp_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp);

    virtual void bp_error(void const * i_keydata,
                          size_t i_keysize,
                          void const * i_argp,
                          utp::Exception const & i_exp);

    // VBSPutRequest

    virtual void process(VBSChild * i_cp,
                         utp::BlockStoreHandle const & i_bsh);
                         

private:
    utp::OctetSeq							m_key;
    utp::OctetSeq							m_blk;
    utp::BlockStore::BlockPutCompletion *	m_cmpl;
    void const *							m_argp;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSPutRequest_h__
