#include <iostream>
#include <sstream>
#include "Except.h"
#include "MD5.h"

#include "S3BlockStore.h"
#include "S3AsyncPutHandler.h"
#include "s3bslog.h"
#include "s3bsfwd.h"

using namespace std;
using namespace utp;

namespace S3BS {

AsyncPutHandler::AsyncPutHandler(ACE_Reactor * i_reactor,
                                 S3BlockStore & i_s3bs,
                                 string const & i_blkpath,
                                 void const * i_keydata,
                                 size_t i_keysize,
                                 uint8 const * i_blkdata,
                                 size_t i_blksize,
                                 BlockStore::BlockPutCompletion & i_cmpl,
                                 void const * i_argp,
                                 size_t i_retries)
    : PutHandler(i_blkdata, i_blksize)
    , m_reactor(i_reactor)
    , m_s3bs(i_s3bs)
    , m_blkpath(i_blkpath)
    , m_keydata(i_keydata)
    , m_keysize(i_keysize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
    , m_retries(i_retries)
    , m_md5(i_blkdata, i_blksize)
{
    LOG(lgr, 6, (void *) this << ' '
        << keystr(m_keydata, m_keysize) << " CTOR sz=" << i_blksize);

    ACE_OS::memset(&m_pp, '\0', sizeof(m_pp));
    m_pp.md5 = m_md5;

    this->reference_counting_policy().value
        (ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

AsyncPutHandler::~AsyncPutHandler()
{
    LOG(lgr, 6, (void *) this << ' '
        << keystr(m_keydata, m_keysize) << " DTOR");
}

void
AsyncPutHandler::rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
{
    // Call base class completion first.
    PutHandler::rh_complete(status, errorDetails);

    // IMPORTANT - We can be called here w/ the S3BlockStore mutex
    // held so we'll need to enqueue all the goods and put called from
    // the reactor without the mutex held ...
    m_reactor->notify(this);
}

ACE_Event_Handler::Reference_Count
AsyncPutHandler::add_reference()
{
    LOG(lgr, 9, (void *) this << ' '
        << "add_reference " << rc_count() << "->" << (rc_count() + 1));

    return this->rc_add_ref();
}

ACE_Event_Handler::Reference_Count
AsyncPutHandler::remove_reference()
{
    LOG(lgr, 9, (void *) this << ' '
        << "remove_reference " << rc_count() << "->" << (rc_count() - 1));

    // Don't touch any members after this!
    return this->rc_rem_ref();
}

int
AsyncPutHandler::handle_exception(ACE_HANDLE fd)
{
    S3Status st = status();

    // Was this a successful completion?
    if (st == S3StatusOK)
    {
        LOG(lgr, 6, (void *) this << ' '
            << keystr(m_keydata, m_keysize) << " SUCCESS");

        // Update the blockstore accounting.
        m_s3bs.update_put_stats(this);

        // Call the completion handler.
        m_cmpl.bp_complete(m_keydata, m_keysize, m_argp);

        // IMPORTANT - We put destructed here; Don't touch *anything*
        // after this!
        //
        m_s3bs.remove_handler(this);
    }

    // Retryable failure?
    else if (S3_status_is_retryable(st))
    {
        if (--m_retries > 0)
        {
            LOG(lgr, 3, (void *) this << ' '
                << keystr(m_keydata, m_keysize) << ": " << st << ": RETRY");

            reset(); // Reset our state.
            m_s3bs.initiate_put(this);

            // This path doesn't destroy the handler ...
        }
        else
        {
            LOG(lgr, 2, (void *) this << ' '
                << keystr(m_keydata, m_keysize)  << ": " << st
                << " TOO MANY RETRIES");

            ostringstream errstrm;
            errstrm << FILELINE << "too many S3 retries";
            InternalError ex(errstrm.str().c_str());
            m_cmpl.bp_error(m_keydata, m_keysize, m_argp, ex);

            // IMPORTANT - We put destructed here; Don't touch *anything*
            // after this!
            //
            m_s3bs.remove_handler(this);
        }
    }

    // Permanent failure ... bitter ...
    else
    {
        // Call the error handler.
        ostringstream errstrm;
        LOG(lgr, 6, (void *) this << ' '
            << keystr(m_keydata, m_keysize) << " UNEXPECTED: " << st);

        errstrm << FILELINE << "unexpected S3 error: " << st;
        InternalError ex(errstrm.str().c_str());
        m_cmpl.bp_error(m_keydata, m_keysize, m_argp, ex);

        // IMPORTANT - We put destructed here; Don't touch *anything*
        // after this!
        //
        m_s3bs.remove_handler(this);
    }

    return 0;
}

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
