#include <iostream>
#include <sstream>
#include "Except.h"
#include "MD5.h"

#include <ace/Handle_Set.h>

#include "S3BlockStore.h"
#include "s3bsfwd.h"
#include "s3bslog.h"
#include "S3BucketDestroyer.h"

using namespace std;
using namespace utp;
using namespace S3BS;

namespace {

S3Status response_properties(S3ResponseProperties const * properties,
                             void * callbackData)
{
    ResponseHandler * rhp = (ResponseHandler *) callbackData;
    return rhp->rh_properties(properties);
}

void response_complete(S3Status status,
                       S3ErrorDetails const * errorDetails,
                       void * callbackData)
{
    ResponseHandler * rhp = (ResponseHandler *) callbackData;
    rhp->rh_complete(status, errorDetails);
}

S3ResponseHandler rsp_tramp = { response_properties, response_complete };

}

namespace S3BS {

ObjectDestroyer::ObjectDestroyer(BucketDestroyer & i_bd,
                                 string const & i_key,
                                 size_t i_retries)
    : m_bd(i_bd)
    , m_key(i_key)
    , m_reactor(ACE_Reactor::instance())
    , m_retries(i_retries)
{
    LOG(lgr, 6, (void *) this << " CTOR " << m_key);

    this->reference_counting_policy().value
        (ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

    // Hold a ref to ourselves until we're complete ...
    m_self = this;
}

ObjectDestroyer::~ObjectDestroyer()
{
    LOG(lgr, 6, (void *) this << " DTOR " << m_key);
}

void
ObjectDestroyer::rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
{
    LOG(lgr, 6, (void *) this << " DESTROY " << m_key << " COMPLETE");

    // Call base class completion first.
    ResponseHandler::rh_complete(status, errorDetails);
    m_reactor->notify(this);
}

ACE_Event_Handler::Reference_Count
ObjectDestroyer::add_reference()
{
    LOG(lgr, 9, (void *) this << ' '
        << "add_reference " << rc_count() << "->" << (rc_count() + 1));

    return this->rc_add_ref();
}

ACE_Event_Handler::Reference_Count
ObjectDestroyer::remove_reference()
{
    LOG(lgr, 9, (void *) this << ' '
        << "remove_reference " << rc_count() << "->" << (rc_count() - 1));

    // Don't touch any members after this!
    return this->rc_rem_ref();
}

int
ObjectDestroyer::handle_exception(ACE_HANDLE fd)
{
    S3Status st = status();

    // Was this a successful completion?
    if (st == S3StatusOK)
    {
        LOG(lgr, 5, "DESTROY " << m_key << " SUCCESS");

        m_bd.completed();

        // Don't touch anything after this, likely we go away ...
        m_self = NULL;
    }

    // Retryable failure?
    else if (S3_status_is_retryable(st))
    {
        if (--m_retries > 0)
        {
            LOG(lgr, 3, (void *) this << " DESTROY " << m_key
                << ": " << st << ": RETRY");

            reset(); // Reset our state.
            m_bd.initiate_delete(this);

            // This path doesn't destroy the handler ...
        }
        else
        {
            LOG(lgr, 3, (void *) this << " DESTROY " << m_key
                << ": " << st << ": TOO MANY RETRIES");

            m_bd.completed();

            // Don't touch anything after this, likely we go away ...
            m_self = NULL;
        }
    }

    // Permanent failure ... bitter ...
    else
    {
        LOG(lgr, 2, (void *) this << " DESTROY " << m_key
            << ": " << st << ": UNEXPECTED");

        m_bd.completed();

        // Don't touch anything after this, likely we go away ...
        m_self = NULL;
    }

    return 0;
}

// NOTE - Unfortunately this class duplicates a lot of the features in
// the S3BlockStore object.  But it is used when we don't actually
// want to create a blockstore instance (we are destroying one) ...


BucketDestroyer::BucketDestroyer(S3BucketContext & i_buckctxt)
    : m_istrunc(false)
    , m_buckctxt(i_buckctxt)
    , m_reactor(ACE_Reactor::instance())
    , m_reqctxt(NULL)
    , m_outstanding(0)
{
    LOG(lgr, 4, "BucketDestroyer creating S3 request context");

    S3Status st = S3_create_request_context(&m_reqctxt);
    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "unexpected S3 error: " << st);
}

BucketDestroyer::~BucketDestroyer()
{
    // Unregister any request context handlers.
    LOG(lgr, 6, "BucketDestroyer unregistering handlers");
    if (m_rset.num_set() > 0)
        m_reactor->remove_handler(m_rset, ACE_Event_Handler::READ_MASK |
                                          ACE_Event_Handler::DONT_CALL);

    if (m_wset.num_set() > 0)
        m_reactor->remove_handler(m_wset, ACE_Event_Handler::WRITE_MASK |
                                          ACE_Event_Handler::DONT_CALL);
        
    if (m_eset.num_set() > 0)
        m_reactor->remove_handler(m_eset, ACE_Event_Handler::EXCEPT_MASK |
                                          ACE_Event_Handler::DONT_CALL);
        
    if (m_reqctxt)
    {
        LOG(lgr, 6, "BucketDestroyer destroying S3 request context");
        S3_destroy_request_context(m_reqctxt);
        m_reqctxt = NULL;
    }
}

S3Status
BucketDestroyer::lh_item(int i_istrunc,
                         char const * i_next_marker,
                         int i_contents_count,
                         S3ListBucketContent const * i_contents,
                         int i_common_prefixes_count,
                         char const ** i_common_prefixes)
{
    m_istrunc = i_istrunc;

    if (i_common_prefixes_count)
        throwstream(InternalError, FILELINE
                    << "common prefixes make me sad");

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_bdmutex);

        for (int i = 0; i < i_contents_count; ++i)
        {
            S3ListBucketContent const * cp = &i_contents[i];
            m_keyqueue.push(cp->key);
            LOG(lgr, 6, "BucketDestroyer enqueued " << cp->key);
        }
    }

    m_last_seen = i_contents_count ?
        i_contents[i_contents_count-1].key : "";

    service_queue();

    return S3StatusOK;
}

int
BucketDestroyer::handle_input(ACE_HANDLE i_fd)
{
    LOG(lgr, 6, "BucketDestroyer::handle_input");
    return reqctxt_service();
}

int
BucketDestroyer::handle_output(ACE_HANDLE i_fd)
{
    LOG(lgr, 6, "BucketDestroyer::handle_output");
    return reqctxt_service();
}

int
BucketDestroyer::handle_exception(ACE_HANDLE i_fd)
{
    LOG(lgr, 6, "BucketDestroyer::handle_exception");
    return reqctxt_service();
}

int
BucketDestroyer::handle_timeout(ACE_Time_Value const & current_time,
                                void const * act)
{
    LOG(lgr, 6, "BucketDestroyer::handle_timeout");
    return reqctxt_service();
}

void
BucketDestroyer::initiate_delete(ObjectDestroyerHandle const & i_odh)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_bdmutex);

    S3_delete_object(&m_buckctxt,
                     i_odh->key().c_str(),
                     m_reqctxt,
                     &rsp_tramp,
                     &*i_odh);

    // Seems we need to advance the state w/ this non-blocking
    // call before we can register FDs w/ the reactor?
    int nreqremain;
    S3Status st = S3_runonce_request_context(m_reqctxt,
                                             &nreqremain);

    // If things aren't good log and keep going ...
    if (st != S3StatusOK)
    {
        LOG(lgr, 2, "BucketDestroyer "
            << "initiate_put_internal: S3_runonce_request_context: " << st);
    }

    // Re-register the request context.
    reqctxt_reregister();

}

int
BucketDestroyer::reqctxt_service()
{
    LOG(lgr, 6, "BucketDestroyer reqctxt_service starting");

    ACE_Guard<ACE_Thread_Mutex> guard(m_bdmutex);

    int nreqremain;
    S3Status st = S3_runonce_request_context(m_reqctxt,
                                             &nreqremain);

    // If things aren't good log and keep going ...
    if (st != S3StatusOK)
    {
        LOG(lgr, 2, "BucketDestroyer "
            << "reqctxt_service: S3_runonce_request_context: " << st);
    }

    // Stuff might have changed ...
    reqctxt_reregister();

    LOG(lgr, 6, "BucketDestroyer "
        << "reqctxt_service finished nreqremain=" << nreqremain);

    return 0;
}

void
BucketDestroyer::reqctxt_reregister()
{
    // IMPORTANT - It's presumed that the called of this routine
    // has the mutex held ...

    LOG(lgr, 6, "BucketDestroyer reqctxt_reregister starting");

    // Cancel any existing timeouts.
    m_reactor->cancel_timer(this);

    // Cancel any existing registrations.
    LOG(lgr, 6, "BucketDestroyer unregistering handlers");

    if (m_rset.num_set() > 0)
        m_reactor->remove_handler(m_rset, ACE_Event_Handler::READ_MASK |
                                  ACE_Event_Handler::DONT_CALL);

    if (m_wset.num_set() > 0)
        m_reactor->remove_handler(m_wset, ACE_Event_Handler::WRITE_MASK |
                                  ACE_Event_Handler::DONT_CALL);
        
    if (m_eset.num_set() > 0)
        m_reactor->remove_handler(m_eset, ACE_Event_Handler::EXCEPT_MASK |
                                  ACE_Event_Handler::DONT_CALL);
        

    // Fill the fd_set's w/ the fds the reqctxt is using.
    int maxfd;
    fd_set rfdset; FD_ZERO(&rfdset);
    fd_set wfdset; FD_ZERO(&wfdset);
    fd_set efdset; FD_ZERO(&efdset);
    S3Status st = S3_get_request_context_fdsets(m_reqctxt,
                                                &rfdset,
                                                &wfdset,
                                                &efdset,
                                                &maxfd);

    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE << "unexpected status: " << st);

    m_rset = ACE_Handle_Set(rfdset);
    m_wset = ACE_Handle_Set(wfdset);
    m_eset = ACE_Handle_Set(efdset);

    LOG(lgr, 6, "BucketDestroyer registering handlers");

    if (m_eset.num_set() > 0)
        m_reactor->register_handler(m_eset,
                                    this,
                                    ACE_Event_Handler::EXCEPT_MASK);

    if (m_wset.num_set() > 0)
        m_reactor->register_handler(m_wset,
                                    this,
                                    ACE_Event_Handler::WRITE_MASK);

    if (m_rset.num_set() > 0)
        m_reactor->register_handler(m_rset,
                                    this,
                                    ACE_Event_Handler::READ_MASK);

    // Set a timeout.
    int64_t maxmsec = S3_get_request_context_timeout(m_reqctxt);
    if (maxmsec >= 0)
    {
        time_t secs = maxmsec / 1000;
        suseconds_t usecs = (maxmsec % 1000) * 1000;
        ACE_Time_Value to(secs, usecs);
        m_reactor->schedule_timer(this, NULL, to);
    }

    LOG(lgr, 6, "BucketDestroyer reqctxt_reregister finished");
}

void
BucketDestroyer::completed()
{
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_bdmutex);
        --m_outstanding;
    }

    service_queue();
}

void
BucketDestroyer::service_queue()
{
    // We only make a limited number of requests at a time.

    ACE_Guard<ACE_Thread_Mutex> guard(m_bdmutex);

    while (m_outstanding < 128 && !m_keyqueue.empty())
    {
        ++m_outstanding;
        string key = m_keyqueue.front();
        m_keyqueue.pop();

        LOG(lgr, 6, "BucketDestroyer dequeued " << key);

        // Release the lock while we initiate the ObjectDestroyer ...
        ACE_Reverse_Lock<ACE_Thread_Mutex> revmutex(m_bdmutex);
        ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > unguard(revmutex);

        ObjectDestroyerHandle odh = new ObjectDestroyer(*this, key, 10);
        initiate_delete(odh);
    }
}

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
