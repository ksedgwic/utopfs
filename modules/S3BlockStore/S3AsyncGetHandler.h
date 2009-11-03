#ifndef S3AsyncGetHandler_h__
#define S3AsyncGetHandler_h__

/// @file S3AsyncGetHandler.h
/// FileSystem S3 Async Get Object.

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

class S3BS_EXP AsyncGetHandler
    : public GetHandler
    , public ACE_Event_Handler
{
public:
    AsyncGetHandler(ACE_Reactor * i_reactor,
                    S3BlockStore & i_s3bs,
                    std::string const & i_blkpath,
                    void const * i_keydata,
                    size_t i_keysize,
                    utp::uint8 * o_buffdata,
                    size_t i_buffsize,
                    utp::BlockStore::BlockGetCompletion & i_cmpl,
                    void const * i_argp,
                    size_t i_retries);

    virtual ~AsyncGetHandler();

    // GetHandler methods

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails);

    // ACE_Event_Handler methods

    virtual Reference_Count add_reference();

    virtual Reference_Count remove_reference();

    virtual int handle_exception(ACE_HANDLE fd);

    // AsyncGetHandler methods

    std::string const & blkpath() const { return m_blkpath; }

private:
    ACE_Reactor *							m_reactor;
    S3BlockStore &							m_s3bs;
    std::string								m_blkpath;
    void const *							m_keydata;
    size_t									m_keysize;
    utp::BlockStore::BlockGetCompletion &	m_cmpl;
    void const *							m_argp;
    size_t									m_retries;
};
typedef utp::RCPtr<AsyncGetHandler> AsyncGetHandlerHandle;


} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3AsyncGetHandler_h__
