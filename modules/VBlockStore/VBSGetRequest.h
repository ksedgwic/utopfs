#ifndef VBSGetRequest_h__
#define VBSGetRequest_h__

/// @file VBSGetRequest.h
/// Virtual BlockStore Get Request

#include "VBSRequest.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSGetRequest
    : public VBSRequest
    , public utp::BlockStore::BlockGetCompletion
{
public:
    VBSGetRequest(VBlockStore & i_vbs,
                  long i_outstanding,
                  void const * i_keydata,
                  size_t i_keysize,
                  void * o_buffdata,
                  size_t i_buffsize,
                  utp::BlockStore::BlockGetCompletion * i_cmpl,
                  void const * i_argp);

    virtual ~VBSGetRequest();

    // VBSRequest

    virtual void stream_insert(std::ostream & ostrm) const;

    virtual void process(VBSChild * i_cp,
                         utp::BlockStoreHandle const & i_bsh);

    // BlockGetCompletion

    virtual void bg_complete(void const * i_keydata,
                             size_t i_keysize,
                             void const * i_argp,
                             size_t i_blksize);

    virtual void bg_error(void const * i_keydata,
                          size_t i_keysize,
                          void const * i_argp,
                          utp::Exception const & i_exp);

    // VBSGetRequest

    void needy(VBSChildHandle const & i_needy);

private:
    utp::OctetSeq							m_key;
    utp::OctetSeq							m_blk;
    void *									m_buffdata;
    size_t									m_buffsize;
    utp::BlockStore::BlockGetCompletion *	m_cmpl;
    void const *							m_argp;
    size_t									m_retsize;
    VBSChildSeq								m_needy;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSGetRequest_h__
