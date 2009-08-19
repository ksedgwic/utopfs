#ifndef VBSRequest_h__
#define VBSRequest_h__

/// @file VBSRequest.h
/// Virtual BlockStore Request Base Class

#include <iosfwd>

#include <ace/Thread_Mutex.h>

#include "BlockStore.h"
#include "utpfwd.h"

#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore Request Base Class
//
class VBS_EXP VBSRequest : public virtual utp::RCObj
{
public:
    VBSRequest(VBlockStore & i_vbs, long i_outstanding);

    virtual ~VBSRequest();

    virtual bool operator<(VBSRequest const & i_o) const;

    virtual void stream_insert(std::ostream & ostrm) const = 0;

    virtual void done();

protected:
    VBlockStore &						m_vbs;

    ACE_Thread_Mutex					m_vbsreqmutex;
    bool								m_succeeded;
    long								m_outstanding;
};

std::ostream & operator<<(std::ostream & ostrm, VBSRequest const & i_req);

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
                  utp::BlockStore::BlockPutCompletion & i_cmpl,
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
    utp::BlockStore::BlockPutCompletion &	m_cmpl;
    void const *							m_argp;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSRequest_h__
