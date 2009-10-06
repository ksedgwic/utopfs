#ifndef S3AsyncPutHandler_h__
#define S3AsyncPutHandler_h__

/// @file S3AsyncPutHandler.h
/// FileSystem S3 Async Put Object.

#include <ace/Event_Handler.h>
#include <ace/Reactor.h>

#include <libs3.h>

#include "utpfwd.h"
#include "BlockStore.h"
#include "Types.h"
#include "RC.h"

#include "S3ResponseHandler.h"
#include "s3bsexp.h"
#include "s3bsfwd.h"

namespace S3BS {

class S3BS_EXP AsyncPutHandler
    : public PutHandler
    , ACE_Event_Handler
{
public:
    AsyncPutHandler(ACE_Reactor * i_reactor,
                    S3BlockStore & i_s3bs,
                    std::string const & i_blkpath,
                    void const * i_keydata,
                    size_t i_keysize,
                    utp::uint8 const * i_blkdata,
                    size_t i_blksize,
                    utp::BlockStore::BlockPutCompletion & i_cmpl,
                    void const * i_argp,
                    size_t i_retries);

    virtual ~AsyncPutHandler();

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails);

    // ACE_Event_Handler methods

    virtual Reference_Count add_reference();

    virtual Reference_Count remove_reference();

    virtual int handle_exception(ACE_HANDLE fd);

    // AsyncPutHandler methods

    std::string const & blkpath() const { return m_blkpath; }
    
    S3PutProperties const * ppp() const { return &m_pp; }

private:
    ACE_Reactor *							m_reactor;
    S3BlockStore &							m_s3bs;
    std::string								m_blkpath;
    void const *							m_keydata;
    size_t									m_keysize;
    utp::BlockStore::BlockPutCompletion &	m_cmpl;
    void const *							m_argp;
    size_t									m_retries;
    S3PutProperties							m_pp;
};
typedef utp::RCPtr<AsyncPutHandler> AsyncPutHandlerHandle;


} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3AsyncPutHandler_h__
