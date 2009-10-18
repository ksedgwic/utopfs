#include "Except.h"

#include "S3ResponseHandler.h"
#include "s3bslog.h"

using namespace std;
using namespace utp;

namespace S3BS {

// ----------------------------------------------------------------
// ResponseHandler
// ----------------------------------------------------------------

ResponseHandler::ResponseHandler()
    : m_propsvalid(false)
    , m_s3rhcond(m_s3rhmutex)
    , m_waiters(false)
    , m_complete(false)
{
}

S3Status
ResponseHandler::rh_properties(S3ResponseProperties const * pp)
{
    m_propsvalid = true;

    if (pp->requestId)
        m_reqid = pp->requestId;
    if (pp->requestId2)
        m_reqid2 = pp->requestId2;
    if (pp->contentType)
        m_content_type = pp->contentType;
    m_content_length = pp->contentLength;
    if (pp->server)
        m_server = pp->server;
    if (pp->eTag)
        m_etag = pp->eTag;

    m_last_modified = pp->lastModified;

    for (int i = 0; i < pp->metaDataCount; ++i)
    {
        S3NameValue const * nvp = &pp->metaData[i];
        m_metadata.push_back(make_pair(nvp->name, nvp->value));
    }

    return S3StatusOK;
}

void
ResponseHandler::rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_s3rhmutex);
    m_status = status;
    m_complete = true;
    if (m_waiters)
        m_s3rhcond.broadcast();
}

void
ResponseHandler::reset()
{
    m_waiters = false;
    m_complete = false;
}

S3Status
ResponseHandler::wait()
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_s3rhmutex);
    m_waiters = true;
    while (!m_complete)
        m_s3rhcond.wait();
    return m_status;
}

S3Status
ResponseHandler::status() const
{
    return m_status;
}

bool
ResponseHandler::operator<(ResponseHandler const & i_o) const
{
    // Just use the address of the request.
    return this < &i_o;
}

bool
ResponseHandler::operator==(ResponseHandler const & i_o) const
{
    // Just use the address of the request.
    return this == &i_o;
}

// ----------------------------------------------------------------
// PutHandler
// ----------------------------------------------------------------

PutHandler::PutHandler(uint8 const * i_data, size_t i_size)
    : m_data(i_data)
    , m_size(i_size)
    , m_left(i_size)
{
    LOG(lgr, 7, "PutHandler " << (void *) this << " CTOR sz=" << i_size);
}

PutHandler::~PutHandler()
{
    LOG(lgr, 7, "PutHandler " << (void *) this << " DTOR");
}

int
PutHandler::ph_objdata(int i_buffsz, char * o_buffer)
{
    size_t sz = min(size_t(i_buffsz), m_left);

    ACE_OS::memcpy(o_buffer, m_data, sz);
    m_data += sz;
    m_left -= sz;
    return sz;
}

// ----------------------------------------------------------------
// GetHandler
// ----------------------------------------------------------------

GetHandler::GetHandler(uint8 * o_buffdata, size_t i_buffsize)
    : m_buffdata(o_buffdata)
    , m_buffsize(i_buffsize)
    , m_size(0)
{}

S3Status
GetHandler::gh_objdata(int i_buffsz, char const * i_buffer)
{
    size_t sz = min(size_t(i_buffsz), (m_buffsize - m_size));
    ACE_OS::memcpy(&m_buffdata[m_size], i_buffer, sz);
    m_size += sz;
    return S3StatusOK;
}

size_t
GetHandler::size() const
{
    return m_size;
} 

// ----------------------------------------------------------------
// GetHandler
// ----------------------------------------------------------------

ListHandler::ListHandler()
{
}

S3Status
ListHandler::lh_item(int i_istrunc,
                     char const * i_next_marker,
                     int i_contents_count,
                     S3ListBucketContent const * i_contents,
                     int i_common_prefixes_count,
                     char const ** i_common_prefixes)
{
    if (i_istrunc)
        throwstream(InternalError, FILELINE
                    << "Rats, need to implement truncated handling ...");

    for (int i = 0; i < i_common_prefixes_count; ++i)
    {
        LOG(lgr, 7, "lh_item pfx " << i_common_prefixes[i]);
    }

    for (int i = 0; i < i_contents_count; ++i)
    {
        S3ListBucketContent const * cp = &i_contents[i];
        LOG(lgr, 7, "lh_item " << cp->size << ' ' << cp->key);
    }

    return S3StatusOK;
}

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
