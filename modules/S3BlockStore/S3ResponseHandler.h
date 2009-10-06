#ifndef S3ResponseHandler_h__
#define S3ResponseHandler_h__

/// @file S3ResponseHandler.h
/// FileSystem ResponseHandler Instance.

#include <vector>
#include <string>

#include <libs3.h>

#include "utpfwd.h"

#include "Types.h"
#include "RC.h"

#include "s3bsexp.h"
#include "s3bsfwd.h"

namespace S3BS {

class S3BS_EXP ResponseHandler : public utp::RCObj
{
public:
    ResponseHandler();

    virtual S3Status rh_properties(S3ResponseProperties const * pp);

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails);

    void reset();	// Resets state for multiple uses.

    S3Status wait();

    S3Status status() const;

    // Comparison operator for collection operations.
    bool operator<(ResponseHandler const & i_o) const;

    // Equality operator for collection operations.
    bool operator==(ResponseHandler const & i_o) const;

    typedef std::vector<std::pair<std::string, std::string> > NameValueSeq;

    // From the S3ResponseProperties
    bool						m_propsvalid;
    std::string					m_reqid;
    std::string					m_reqid2;
    std::string					m_content_type;
    uint64_t					m_content_length;
    std::string					m_server;
    std::string					m_etag;
    int64_t						m_last_modified;
    NameValueSeq				m_metadata;

private:
    ACE_Thread_Mutex			m_s3rhmutex;
    ACE_Condition_Thread_Mutex	m_s3rhcond;
    bool						m_waiters;
    bool						m_complete;
    S3Status					m_status;
};

class S3BS_EXP PutHandler : public ResponseHandler
{
public:
    PutHandler(utp::uint8 const * i_data, size_t i_size);

    virtual int ph_objdata(int i_buffsz, char * o_buffer);

    utp::uint8 const * blkdata() const { return m_data; }

    size_t blksize() const { return m_size; }

private:
    utp::uint8 const *		m_data;
    size_t					m_size;
};

class S3BS_EXP GetHandler : public ResponseHandler
{
public:
    GetHandler(utp::uint8 * o_buffdata, size_t i_buffsize);

    virtual S3Status gh_objdata(int i_buffsz, char const * i_buffer);

    size_t size() const;

private:
    utp::uint8 *		m_buffdata;
    size_t				m_buffsize;
    size_t				m_size;
};

class S3BS_EXP ListHandler : public ResponseHandler
{
public:
    ListHandler();

    virtual S3Status lh_item(int i_istrunc,
                             char const * i_next_marker,
                             int i_contents_count,
                             S3ListBucketContent const * i_contents,
                             int i_common_prefixes_count,
                             char const ** i_common_prefixes);
private:
};

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3ResponseHandler_h__
