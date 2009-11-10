#include <iostream>
#include <sstream>
#include "Except.h"

#include "S3BlockStore.h"
#include "S3AsyncGetHandler.h"
#include "s3bslog.h"
#include "s3bsfwd.h"

using namespace std;
using namespace utp;

namespace S3BS {

AsyncGetHandler::AsyncGetHandler(ACE_Reactor * i_reactor,
                                 S3BlockStore & i_s3bs,
                                 string const & i_blkpath,
                                 void const * i_keydata,
                                 size_t i_keysize,
                                 uint8 * o_buffdata,
                                 size_t i_buffsize,
                                 BlockStore::BlockGetCompletion & i_cmpl,
                                 void const * i_argp,
                                 size_t i_retries)
    : GetHandler(o_buffdata, i_buffsize)
    , m_reactor(i_reactor)
    , m_s3bs(i_s3bs)
    , m_blkpath(i_blkpath)
    , m_keydata(i_keydata)
    , m_keysize(i_keysize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
    , m_retries(i_retries)
{
    LOG(lgr, 6, (void *) this << ' '
        << keystr(m_keydata, m_keysize) << " CTOR");

    this->reference_counting_policy().value
        (ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

AsyncGetHandler::~AsyncGetHandler()
{
    LOG(lgr, 6, (void *) this << ' '
        << keystr(m_keydata, m_keysize) << " DTOR");
}

void
AsyncGetHandler::rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
{
    // Call base class completion first.
    GetHandler::rh_complete(status, errorDetails);

    // IMPORTANT - We can be called here w/ the S3BlockStore mutex
    // held so we'll need to enqueue all the goods and get called from
    // the reactor without the mutex held ...
    m_reactor->notify(this);
}

ACE_Event_Handler::Reference_Count
AsyncGetHandler::add_reference()
{
    LOG(lgr, 9, "add_reference " << rc_count() << "->" << (rc_count() + 1));

    return this->rc_add_ref();
}

ACE_Event_Handler::Reference_Count
AsyncGetHandler::remove_reference()
{
    LOG(lgr, 9, "remove_reference " << rc_count() << "->" << (rc_count() - 1));

    // Don't touch any members after this!
    return this->rc_rem_ref();
}

int
AsyncGetHandler::handle_exception(ACE_HANDLE fd)
{
    S3Status st = status();
    
    // Was this a successful completion?
    if (st == S3StatusOK)
    {
        LOG(lgr, 6, (void *) this << ' '
            << keystr(m_keydata, m_keysize) << " SUCCESS");

        // Call the completion handler.
        m_cmpl.bg_complete(m_keydata, m_keysize, m_argp, size());

        // IMPORTANT - We get destructed here; Don't touch *anything*
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

            rh_reset(); // Reset our state.
            m_s3bs.initiate_get(this);

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
            m_cmpl.bg_error(m_keydata, m_keysize, m_argp, ex);

            // IMPORTANT - We get destructed here; Don't touch *anything*
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
        if (st == S3StatusErrorNoSuchKey)
        {
            LOG(lgr, 2, (void *) this << ' '
                << keystr(m_keydata, m_keysize) << " NOT FOUND");

            errstrm << "key \"" << m_blkpath << "\" not found";
            NotFoundError ex(errstrm.str().c_str());
            m_cmpl.bg_error(m_keydata, m_keysize, m_argp, ex);
        }
        else
        {
            LOG(lgr, 2, (void *) this << ' '
                << keystr(m_keydata, m_keysize) << "UNEXPECTED: " << st);

            errstrm << FILELINE << "unexpected S3 error: " << st;
            InternalError ex(errstrm.str().c_str());
            m_cmpl.bg_error(m_keydata, m_keysize, m_argp, ex);
        }

        // IMPORTANT - We get destructed here; Don't touch *anything*
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
