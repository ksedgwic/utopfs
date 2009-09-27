#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>
#include <ace/Dirent.h>
#include <ace/os_include/os_byteswap.h>

#include "Base32.h"
#include "Base64.h"
#include "BlockStoreFactory.h"
#include "Digest.h"
#include "Log.h"
#include "MD5.h"

#include "MDIndex.pb.h"

#include "S3BlockStore.h"
#include "s3bslog.h"

using namespace std;
using namespace utp;

namespace S3BS {

static unsigned const MAX_RETRIES = 10;

std::ostream & operator<<(std::ostream & ostrm, S3Status status)
{
    ostrm << S3_get_status_name(status);
    return ostrm;
}
    
class ResponseHandler
{
public:
    ResponseHandler()
        : m_propsvalid(false)
        , m_s3rhcond(m_s3rhmutex)
        , m_complete(false)
    {
    }

    virtual S3Status rh_properties(S3ResponseProperties const * pp)
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

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_s3rhmutex);
        m_status = status;
        m_complete = true;
        m_s3rhcond.broadcast();
    }

    S3Status wait()
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_s3rhmutex);
        while (!m_complete)
            m_s3rhcond.wait();
        return m_status;
    }

    typedef vector<pair<string, string> > NameValueSeq;

    // From the S3ResponseProperties
    bool						m_propsvalid;
    string						m_reqid;
    string						m_reqid2;
    string						m_content_type;
    uint64_t					m_content_length;
    string						m_server;
    string						m_etag;
    int64_t						m_last_modified;
    NameValueSeq				m_metadata;

private:
    ACE_Thread_Mutex			m_s3rhmutex;
    ACE_Condition_Thread_Mutex	m_s3rhcond;
    bool						m_complete;
    S3Status					m_status;
};

class PutHandler : public ResponseHandler
{
public:
    PutHandler(uint8 const * i_data, size_t i_size)
        : m_data(i_data)
        , m_size(i_size)
    {
    }

    virtual int ph_objdata(int i_buffsz, char * o_buffer)
    {
        size_t sz = min(size_t(i_buffsz), m_size);

        ACE_OS::memcpy(o_buffer, m_data, sz);
        m_data += sz;
        m_size -= sz;
        return sz;
    }

private:
    uint8 const *		m_data;
    size_t				m_size;
};

class GetHandler : public ResponseHandler
{
public:
    GetHandler(uint8 * o_buffdata, size_t i_buffsize)
        : m_buffdata(o_buffdata)
        , m_buffsize(i_buffsize)
        , m_size(0)
    {}

    virtual S3Status gh_objdata(int i_buffsz, char const * i_buffer)
    {
        size_t sz = min(size_t(i_buffsz), (m_buffsize - m_size));
        ACE_OS::memcpy(&m_buffdata[m_size], i_buffer, sz);
        m_size += sz;
        return S3StatusOK;
    }

    size_t size() const { return m_size; }

private:
    uint8 *				m_buffdata;
    size_t				m_buffsize;
    size_t				m_size;
};

class ListHandler : public ResponseHandler
{
public:
    ListHandler() {}

    virtual S3Status lh_item(int i_istrunc,
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

private:
};

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

int put_objdata(int bufferSize, char * buffer, void * callbackData)
{
    PutHandler * php = (PutHandler *) callbackData;
    return php->ph_objdata(bufferSize, buffer);
}

S3Status get_objdata(int bufferSize, char const * buffer, void * callbackData)
{
    GetHandler * ghp = (GetHandler *) callbackData;
    return ghp->gh_objdata(bufferSize, buffer);
}

S3Status list_callback(int isTruncated,
                       const char *nextMarker,
                       int contentsCount, 
                       const S3ListBucketContent *contents,
                       int commonPrefixesCount,
                       const char **commonPrefixes,
                       void *callbackData)
{
    ListHandler * lhp = (ListHandler *) callbackData;
    return lhp->lh_item(isTruncated,
                        nextMarker,
                        contentsCount,
                        contents,
                        commonPrefixesCount,
                        commonPrefixes);
}

S3ResponseHandler rsp_tramp = { response_properties, response_complete };

S3PutObjectHandler put_tramp = { rsp_tramp, put_objdata };

S3GetObjectHandler get_tramp = { rsp_tramp, get_objdata };

S3ListBucketHandler lst_tramp = { rsp_tramp, list_callback };

bool S3BlockStore::c_s3inited = false;

bool
lessByKey(EntryHandle const & i_a, EntryHandle const & i_b)
{
    return i_a->m_name < i_b->m_name;
}

bool
lessByTstamp(EntryHandle const & i_a, EntryHandle const & i_b)
{
    return i_a->m_tstamp < i_b->m_tstamp;
}

class KeyListHandler : public ListHandler
{
public:
    KeyListHandler(StringSeq & o_keys)
        : m_istrunc(false)
        , m_keys(o_keys)
    {}

    virtual S3Status lh_item(int i_istrunc,
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

        for (int i = 0; i < i_contents_count; ++i)
        {
            S3ListBucketContent const * cp = &i_contents[i];
            m_keys.push_back(cp->key);
        }

        m_last_seen = i_contents_count ?
            i_contents[i_contents_count-1].key : "";

        return S3StatusOK;
    }
    
    bool m_istrunc;
    string m_last_seen;

private:
    StringSeq & m_keys;
};

void
S3BlockStore::destroy(StringSeq const & i_args)
{
    // This is painful, but at least we can make it safer then the old
    // "rm -rf" approach.
    //
    // We loop over all the elements of the blockstore and remove them.
    //
    // Mostly we ignore errors, but not if the top level isn't what we
    // think it is.

    S3Protocol protocol;
    S3UriStyle uri_style;
    string access_key_id;
    string secret_access_key;
    string bucket_name;

    parse_params(i_args,
                 protocol,
                 uri_style,
                 access_key_id,
                 secret_access_key,
                 bucket_name);

    LOG(lgr, 4, "destroy " << bucket_name);

    // Make sure the bucket exists.
    ResponseHandler rh;
    char locstr[128];
    S3_test_bucket(protocol,
                   uri_style,
                   access_key_id.c_str(),
                   secret_access_key.c_str(),
                   bucket_name.c_str(),
                   sizeof(locstr),
                   locstr,
                   NULL,
                   &rsp_tramp,
                   &rh);
    S3Status st = rh.wait();

    if (st == S3StatusErrorNoSuchBucket)
        throwstream(NotFoundError,
                    "bucket " << bucket_name << " doesn't exist");

    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Setup a bucket context.
    S3BucketContext buck;
    buck.bucketName = bucket_name.c_str();
    buck.protocol = protocol;
    buck.uriStyle = uri_style;
    buck.accessKeyId = access_key_id.c_str();
    buck.secretAccessKey = secret_access_key.c_str();

    // Accumulate a list of all the keys.
    StringSeq keys;
    string marker = "";
    bool istrunc = false;
    do
    {
        KeyListHandler klh(keys);

        for (unsigned i = 0; i < MAX_RETRIES; ++i)
        {
            if (i == MAX_RETRIES)
                throwstream(InternalError, FILELINE
                            << "too many retries");

            S3_list_bucket(&buck,
                           NULL,
                           marker.empty() ? NULL : marker.c_str(),
                           NULL,
                           INT_MAX,
                           NULL,
                           &lst_tramp,
                           &klh);
            st = klh.wait();
            if (st == S3StatusOK)
                break;

            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, "list_keys " << bucket_name
                << " ERROR: " << st << " RETRYING");
        }

        istrunc = klh.m_istrunc;
        marker = klh.m_last_seen;
    }
    while (istrunc);

    // Delete all of the keys.
    for (unsigned i = 0; i < keys.size(); ++i)
    {
        LOG(lgr, 7, "deleting " << keys[i]);

        for (unsigned tt = 0; tt < MAX_RETRIES; ++tt)
        {
            if (tt == MAX_RETRIES)
                throwstream(InternalError, FILELINE
                            << "too many retries");

            ResponseHandler rh;
            S3_delete_object(&buck,
                             keys[i].c_str(),
                             NULL,
                             &rsp_tramp,
                             &rh);
            st = rh.wait();
            if (st == S3StatusOK)
                break;

            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, "delete_keys " << bucket_name
                << " ERROR: " << st << " RETRYING");
        }
    }

    // Delete the bucket.
    ResponseHandler rh2;
    S3_delete_bucket(protocol,
                     uri_style,
                     access_key_id.c_str(),
                     secret_access_key.c_str(),
                     bucket_name.c_str(),
                     NULL,
                     &rsp_tramp,
                     &rh2);
    st = rh2.wait();
    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Unfortunately we must poll until it actually goes away ...
    for (unsigned i = 0; true; ++i)
    {
        // Re-init the s3 context; it appears buckets don't appear
        // to go away until we re-initialize ... sigh.
        //
        S3_deinitialize();
        S3_initialize(NULL, S3_INIT_ALL);

        ResponseHandler rh;
        char locstr[128];
        S3_test_bucket(protocol,
                       uri_style,
                       access_key_id.c_str(),
                       secret_access_key.c_str(),
                       bucket_name.c_str(),
                       sizeof(locstr),
                       locstr,
                       NULL,
                       &rsp_tramp,
                       &rh);
        S3Status st = rh.wait();

        // If it's gone we're done
        if (st == S3StatusErrorNoSuchBucket)
            break;

        // Did something else go wrong?
        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);

        if (i >= 100)
            throwstream(InternalError, FILELINE
                        << "polled too many times; bucket still there");

        sleep(1);
        LOG(lgr, 7, "polling destroyed bucket again ...");
    }
}

S3BlockStore::S3BlockStore(std::string const & i_instname)
    : m_instname(i_instname)
    , m_size(0)
    , m_committed(0)
    , m_uncommitted(0)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");
}

S3BlockStore::~S3BlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

string const &
S3BlockStore::bs_instname() const
{
    return m_instname;
}

void
S3BlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    // NOTE - We presume that this routine is externally synchronized
    // and does not need to hold the mutex during construction.

    m_size = i_size;
    m_uncommitted = 0;
    m_committed = 0;

    setup_params(i_args);

    LOG(lgr, 4, m_instname << ' '
        << "bs_create " << i_size << ' ' << m_bucket_name);

    S3CannedAcl acl = S3CannedAclPrivate;
    char const * loc = NULL;

    // Make sure the bucket doesn't already exist.
    ResponseHandler rh;
    char locstr[128];
    S3_test_bucket(m_protocol,
                   m_uri_style,
                   m_access_key_id.c_str(),
                   m_secret_access_key.c_str(),
                   m_bucket_name.c_str(),
                   sizeof(locstr),
                   locstr,
                   NULL,
                   &rsp_tramp,
                   &rh);
    S3Status st = rh.wait();

    if (st == S3StatusOK)
        throwstream(NotUniqueError,
                    "bucket " << m_bucket_name << " already exists");

    if (st != S3StatusErrorNoSuchBucket)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Create the bucket.
    ResponseHandler rh2;
    S3_create_bucket(m_protocol,
                     m_access_key_id.c_str(),
                     m_secret_access_key.c_str(),
                     m_bucket_name.c_str(),
                     acl,
                     loc,
                     NULL,
                     &rsp_tramp,
                     &rh2);
    st = rh2.wait();

    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Poll until the bucket appears.
    for (unsigned i = 0; true; ++i)
    {
        // Re-init the s3 context; it appears buckets don't appear
        // until we re-initialize ... sigh.
        //
        S3_deinitialize();
        S3_initialize(NULL, S3_INIT_ALL);

        ResponseHandler rh;
        char locstr[128];
        S3_test_bucket(m_protocol,
                       m_uri_style,
                       m_access_key_id.c_str(),
                       m_secret_access_key.c_str(),
                       m_bucket_name.c_str(),
                       sizeof(locstr),
                       locstr,
                       NULL,
                       &rsp_tramp,
                       &rh);
        S3Status st = rh.wait();

        // If it's gone we're done
        if (st == S3StatusOK)
            break;

        // Did something else go wrong?
        if (st != S3StatusErrorNoSuchBucket)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);

        if (i >= 100)
            throwstream(InternalError, FILELINE
                        << "polled too many times; bucket still there");

        sleep(1);
        LOG(lgr, 7, m_instname << ' ' << "polling created bucket again ...");
    }

    // Setup a bucket context.
    S3BucketContext buck;
    buck.bucketName = m_bucket_name.c_str();
    buck.protocol = m_protocol;
    buck.uriStyle = m_uri_style;
    buck.accessKeyId = m_access_key_id.c_str();
    buck.secretAccessKey = m_secret_access_key.c_str();

    // Create a SIZE object record.
    ostringstream ostrm;
    ostrm << m_size << endl;
    string const & obj = ostrm.str();
    MD5 md5sum(obj.data(), obj.size());

    S3PutProperties pp;
    ACE_OS::memset(&pp, '\0', sizeof(pp));
    pp.md5 = md5sum;

    for (unsigned i = 0; i <= MAX_RETRIES; ++i)
    {
        PutHandler ph((uint8 const *) obj.data(), obj.size());
        S3_put_object(&buck,
                      "SIZE",
                      obj.size(),
                      &pp,
                      NULL,
                      &put_tramp,
                      &ph);
        st = ph.wait();

        switch (st)
        {
        case S3StatusOK:
            return;

        default:
            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, m_instname << ' ' << "bs_create " << m_bucket_name
                << " ERROR: " << st << " RETRYING");
            break;
        }
    }

    throwstream(InternalError, FILELINE << "too many retries");
}

class EntryListHandler : public ListHandler
{
public:
    EntryListHandler(S3BlockStore::EntrySet & entries,
                     S3BlockStore::EntryTimeSet & ets)
        : m_istrunc(false)
        , m_entries(entries)
        , m_ets(ets)
    {}

    virtual S3Status lh_item(int i_istrunc,
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

        for (int i = 0; i < i_contents_count; ++i)
        {
            S3ListBucketContent const * cp = &i_contents[i];

            string entry = cp->key;
            time_t mtime = time_t(cp->lastModified);
            size_t size = cp->size;

            // BOGUS - there is a terrible TZ problem here!
            mtime -= (7 * 60 * 60);

            // We only want to traverse items in BLOCKS/
            if (entry.substr(0, 7) != S3BlockStore::blockpath())
                continue;

            LOG(lgr, 7, "entry " << entry);

            // Insert into the entries and time-sorted entries sets.
            EntryHandle eh = new Entry(entry, mtime, size);
            m_entries.insert(eh);
            m_ets.insert(eh);
        }

        m_last_seen = i_contents_count ?
            i_contents[i_contents_count-1].key : "";

        return S3StatusOK;
    }

    bool								m_istrunc;
    string								m_last_seen;
    
private:
    S3BlockStore::EntrySet &			m_entries;
    S3BlockStore::EntryTimeSet &		m_ets;
};

class EdgeListHandler : public ListHandler
{
public:
    EdgeListHandler(StringSeq & o_edgekeys)
        : m_istrunc(false)
        , m_edgekeys(o_edgekeys)
    {}

    virtual S3Status lh_item(int i_istrunc,
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

        for (int i = 0; i < i_contents_count; ++i)
        {
            S3ListBucketContent const * cp = &i_contents[i];

            string key = cp->key;

            // We only want to traverse items in EDGES/
            if (key.substr(0, 6) != "EDGES/")
                continue;

            LOG(lgr, 7, "edge " << key);

            m_edgekeys.push_back(key);
        }

        m_last_seen = i_contents_count ?
            i_contents[i_contents_count-1].key : "";

        return S3StatusOK;
    }

    bool					m_istrunc;
    string					m_last_seen;
    
private:
    StringSeq &				m_edgekeys;
};

void
S3BlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    // NOTE - We presume that this routine is externally synchronized
    // and does not need to hold the mutex during construction.

    setup_params(i_args);

    LOG(lgr, 4, m_instname << ' ' << "bs_open " << m_bucket_name);

    // Make sure the bucket exists.
    ResponseHandler rh;
    char locstr[128];
    S3_test_bucket(m_protocol,
                   m_uri_style,
                   m_access_key_id.c_str(),
                   m_secret_access_key.c_str(),
                   m_bucket_name.c_str(),
                   sizeof(locstr),
                   locstr,
                   NULL,
                   &rsp_tramp,
                   &rh);
    S3Status st = rh.wait();

    if (st == S3StatusErrorNoSuchBucket)
        throwstream(NotFoundError,
                    "bucket " << m_bucket_name << " doesn't exist");

    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Setup a bucket context.
    S3BucketContext buck;
    buck.bucketName = m_bucket_name.c_str();
    buck.protocol = m_protocol;
    buck.uriStyle = m_uri_style;
    buck.accessKeyId = m_access_key_id.c_str();
    buck.secretAccessKey = m_secret_access_key.c_str();

    S3GetConditions gc;
    gc.ifModifiedSince = -1;
    gc.ifNotModifiedSince = -1;
    gc.ifMatchETag = NULL;
    gc.ifNotMatchETag = NULL;

    string buffer(1024, '\0');

    GetHandler gh((uint8 *) &buffer[0], buffer.size());

    S3_get_object(&buck,
                  "SIZE",
                  &gc,
                  0,
                  0,
                  NULL,
                  &get_tramp,
                  &gh);

    st = gh.wait();
    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    istringstream istrm(buffer);
    istrm >> m_size;
    LOG(lgr, 4, m_instname << ' ' << "bs_open size=" << m_size);

    // Inventory all existing blocks, insert into entries and
    // time-sorted entries sets.
    //
    LOG(lgr, 4, m_instname << ' ' << "bs_open listing blocks");
    EntryTimeSet ets;
    string marker = "";
    bool istrunc = false;
    do
    {
        EntryListHandler elh(m_entries, ets);
        S3_list_bucket(&buck,
                       NULL,
                       marker.empty() ? NULL : marker.c_str(),
                       NULL,
                       INT_MAX,
                       NULL,
                       &lst_tramp,
                       &elh);
        st = elh.wait();
        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);
        istrunc = elh.m_istrunc;
        marker = elh.m_last_seen;

        LOG(lgr, 4, m_instname << ' '
            << "bs_open listed " << m_entries.size() << " blocks");
    }
    while (istrunc);

    LOG(lgr, 4, m_instname << ' '
        << "bs_open saw " << m_entries.size() << " blocks");

    // Figure out the size of the MDNDX file.
    ssize_t mdndxsz = -1;
    for (unsigned ii = 0; ii < MAX_RETRIES; ++ii)
    {
        if (ii == MAX_RETRIES)
            throwstream(InternalError, FILELINE
                        << "too many retries");

        ResponseHandler rh;
        S3_head_object(&buck,
                       "MDNDX",
                       NULL,
                       &rsp_tramp,
                       &rh);
        S3Status st = rh.wait();
        if (st == S3StatusOK)
        {
            mdndxsz = rh.m_content_length;
            LOG(lgr, 4, m_instname << ' ' << "MDNDX " << mdndxsz << " bytes");
            break;
        }

        if (st == S3StatusHttpErrorNotFound)
        {
            // This is OK, we just don't have a MDNDX file.
            LOG(lgr, 4, m_instname << ' ' << "no MNDX file found");
            break;
        }

        // Sigh ... these we retry a few times ...
        LOG(lgr, 4, "mdndx head " << m_bucket_name
            << " ERROR: " << st << " RETRYING");
    }

    if (mdndxsz != -1)
    {
        LOG(lgr, 4, m_instname << ' ' << "reading MDNDX");

        // Read the mdndx into a buffer.
        string mdndxbuf(mdndxsz, '\0');
        for (unsigned ii = 0; ii < MAX_RETRIES; ++ii)
        {
            if (ii == MAX_RETRIES)
                throwstream(InternalError, FILELINE
                            << "too many retries");

            GetHandler gh((uint8 *) &mdndxbuf[0], mdndxbuf.size());

            S3_get_object(&buck,
                          "MDNDX",
                          &gc,
                          0,
                          0,
                          NULL,
                          &get_tramp,
                          &gh);
        
            S3Status st = gh.wait();
            if (st == S3StatusOK)
            {
                LOG(lgr, 4, m_instname << ' ' << "done reading MDNDX");
                break;
            }

            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, "mdndx get " << m_bucket_name
                << " ERROR: " << st << " RETRYING");
        }

        MDIndex mdndx;
        if (!mdndx.ParseFromString(mdndxbuf))
            throwstream(InternalError, FILELINE << "trouble parsing MDNDX");

        LOG(lgr, 4, m_instname << ' '
            << "parsed " << mdndx.mdentry_size() << " MDNDX entries");

        int32_t timeoff = mdndx.has_timeoff() ? mdndx.timeoff() : 0;
        for (int ii = 0; ii < mdndx.mdentry_size(); ++ii)
        {
            string const & name = mdndx.mdentry(ii).name();
            int32_t mtime = timeoff + mdndx.mdentry(ii).mtime();

            EntryHandle eh = new Entry(name, 0, 0);
            EntrySet::const_iterator pos = m_entries.find(eh);
            if (pos == m_entries.end())
            {
                // If this is the MARK insert it special.
                if (name == "MARK")
                {
                    EntryHandle eh = new Entry(name, mtime, 0);
                    m_entries.insert(eh);
                    ets.insert(eh);
                }
                else
                {
                    LOG(lgr, 2, "expected entry \"" << name << "\" missing");
                }
            }
            else
            {
                (*pos)->m_tstamp = mtime;
            }
        }
    }

    // Unroll the ordered set into the LRU list from recent to oldest.
    //
    off_t committed = 0;
    off_t uncommitted = 0;
    bool mark_seen = false;
    for (EntryTimeSet::reverse_iterator rit = ets.rbegin();
         rit != ets.rend();
         ++rit)
    {
        EntryHandle const & eh = *rit;

        m_lru.push_back(eh);

        // Give each entry an iterator to itself ... 
        eh->m_listpos = m_lru.end();
        eh->m_listpos--;

        // Keep track of the committed size.
        if (mark_seen)
        {
            uncommitted += eh->m_size;
        }
        else
        {
            // Is this the mark?
            if (eh->m_name == "MARK")
            {
                m_mark = eh;
                mark_seen = true;
            }
            else
            {
                committed += eh->m_size;
            }
        }
    }

    m_committed = committed;
    m_uncommitted = uncommitted;

    // Read all of the SignedHeadEdges.

    // Accumulate a list of all the signed edges
    LOG(lgr, 4, m_instname << ' ' << "reading EDGES");
    StringSeq edgekeys;
    marker = "";
    istrunc = false;
    do
    {
        EdgeListHandler elh(edgekeys);
        S3_list_bucket(&buck,
                       NULL,
                       marker.empty() ? NULL : marker.c_str(),
                       NULL,
                       INT_MAX,
                       NULL,
                       &lst_tramp,
                       &elh);
        st = elh.wait();
        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);
        istrunc = elh.m_istrunc;
        marker = elh.m_last_seen;

        LOG(lgr, 4, m_instname << ' '
            << "bs_open listed " << edgekeys.size() << " edges");
    }
    while (istrunc);

    LOG(lgr, 4, m_instname << ' ' << "read " << edgekeys.size() << " EDGES");

    // Read all of the signed edges and insert.
    for (StringSeq::const_iterator it = edgekeys.begin();
         it != edgekeys.end();
         ++it)
    {
        string const & edgekey = *it;

        S3GetConditions gc;
        gc.ifModifiedSince = -1;
        gc.ifNotModifiedSince = -1;
        gc.ifMatchETag = NULL;
        gc.ifNotMatchETag = NULL;

        uint8 buffer[8192];

        GetHandler gh(buffer, sizeof(buffer));

        S3_get_object(&buck,
                      edgekey.c_str(),
                      &gc,
                      0,
                      0,
                      NULL,
                      &get_tramp,
                      &gh);

        S3Status st = gh.wait();

        if (st == S3StatusErrorNoSuchKey)
            throwstream(NotFoundError,
                        "edge \"" << edgekey << "\" not found");

        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);

        string encoded((char const *) buffer, gh.size());
        string data = Base64::decode(encoded);
        SignedHeadEdge she;
        int ok = she.ParseFromString(data);
        if (!ok)
            throwstream(InternalError, FILELINE
                        << " SignedHeadEdge deserialize failed");
        m_lhng.insert_head(she);
    }
}

void
S3BlockStore::bs_close()
    throw(InternalError)
{
    LOG(lgr, 4, m_instname << ' ' << "bs_close");

    // Unregister this instance.
    try
    {
        BlockStoreFactory::unmap(m_instname);
    }
    catch (InternalError const & ex)
    {
        // This we can just rethrow ...
        throw;
    }
    catch (Exception const & ex)
    {
        // These shouldn't happen and need to be converted to
        // InternalError ...
        throw InternalError(ex.what());
    }
}

void
S3BlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "bs_stat");

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
    o_stat.bss_size = m_size;
    o_stat.bss_free = m_size - m_committed;
}

void
S3BlockStore::bs_sync()
    throw(InternalError)
{
}

void
 S3BlockStore::bs_block_get_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void * o_buffdata,
                                 size_t i_buffsize,
                                 BlockGetCompletion & i_cmpl,
                                 void const * i_argp)
    throw(InternalError,
          ValueError)
{
    try
    {
        string entry = entryname(i_keydata, i_keysize);
        string blkpath = blockpath(entry);

        LOG(lgr, 6, m_instname << ' '
            << "bs_block_get " << entry.substr(0, 8) << "...");

        // Do we have this block?
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            EntryHandle meh = new Entry(blkpath, 0, 0);
            EntrySet::const_iterator pos = m_entries.find(meh);
            if (pos == m_entries.end())
                throwstream(NotFoundError,
                            "key \"" << blkpath << "\" not in entries");
        }
            
        // Setup a bucket context.
        S3BucketContext buck;
        buck.bucketName = m_bucket_name.c_str();
        buck.protocol = m_protocol;
        buck.uriStyle = m_uri_style;
        buck.accessKeyId = m_access_key_id.c_str();
        buck.secretAccessKey = m_secret_access_key.c_str();

        S3GetConditions gc;
        gc.ifModifiedSince = -1;
        gc.ifNotModifiedSince = -1;
        gc.ifMatchETag = NULL;
        gc.ifNotMatchETag = NULL;

        GetHandler gh((uint8 *) o_buffdata, i_buffsize);

        S3_get_object(&buck,
                      blkpath.c_str(),
                      &gc,
                      0,
                      0,
                      NULL,
                      &get_tramp,
                      &gh);

        S3Status st = gh.wait();

        if (st == S3StatusErrorNoSuchKey)
            throwstream(NotFoundError,
                        "key \"" << blkpath << "\" not found");

        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);

        i_cmpl.bg_complete(i_keydata, i_keysize, i_argp, gh.size());
    }
    catch (Exception const & ex)
    {
        i_cmpl.bg_error(i_keydata, i_keysize, i_argp, ex);
    }
}

void
S3BlockStore::bs_block_put_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void const * i_blkdata,
                                 size_t i_blksize,
                                 BlockPutCompletion & i_cmpl,
                                 void const * i_argp)
    throw(InternalError,
          ValueError)
{
    try
    {
        string entry = entryname(i_keydata, i_keysize);
        string blkpath = blockpath(entry);

        LOG(lgr, 6, m_instname << ' '
            << "bs_block_put " << entry.substr(0, 8) << "...");

        // Do we alerady have this block?
        bool alreadyhave = false;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            EntryHandle meh = new Entry(blkpath, 0, 0);
            EntrySet::const_iterator pos = m_entries.find(meh);
            if (pos != m_entries.end())
                alreadyhave = true;
        }

        if (alreadyhave)
        {
            i_cmpl.bp_complete(i_keydata, i_keysize, i_argp);
        }

        // We need to determine the current size of the block
        // if it already exists.
        //
        off_t prevsize = 0;
        bool wascommitted = true;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            EntryHandle meh = new Entry(blkpath, 0, 0);
            EntrySet::const_iterator pos = m_entries.find(meh);
            if (pos != m_entries.end())
            {
                prevsize = (*pos)->m_size;

                // Is this block older then the MARK?
                if (m_mark && m_mark->m_tstamp > (*pos)->m_tstamp)
                    wascommitted = false;
            }
        }

        off_t prevcommited = 0;
        if (wascommitted)
            prevcommited = prevsize;

        // How much space will be available for this block?
        off_t avail;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
            avail = m_size - m_committed + prevcommited;
        }

        if (off_t(i_blksize) > avail)
            throwstream(NoSpaceError,
                        "insufficent space: "
                        << avail << " bytes avail, needed " << i_blksize);

        // Do we need to remove uncommitted blocks to make room for this
        // block?
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
            while (m_committed + off_t(i_blksize) +
                   m_uncommitted - prevcommited > m_size)
                purge_uncommitted();
        }
            
        // Setup a bucket context.
        S3BucketContext buck;
        buck.bucketName = m_bucket_name.c_str();
        buck.protocol = m_protocol;
        buck.uriStyle = m_uri_style;
        buck.accessKeyId = m_access_key_id.c_str();
        buck.secretAccessKey = m_secret_access_key.c_str();

        MD5 md5sum(i_blkdata, i_blksize);
        S3PutProperties pp;
        ACE_OS::memset(&pp, '\0', sizeof(pp));
        pp.md5 = md5sum;

        for (unsigned i = 0; i <= MAX_RETRIES; ++i)
        {
            if (i == MAX_RETRIES)
                throwstream(InternalError, FILELINE
                            << "too many retries");

            PutHandler ph((uint8 const *) i_blkdata, i_blksize);
            S3_put_object(&buck,
                          blkpath.c_str(),
                          i_blksize,
                          &pp,
                          NULL,
                          &put_tramp,
                          &ph);
            S3Status st = ph.wait();

            if (st == S3StatusOK)
                break;

            LOG(lgr, 4, m_instname << ' '
                << "bs_block_put_async " << entry.substr(0, 8) << "..."
                << " ERROR: " << st << ", RETRYING");
        }

        time_t mtime = time(NULL);
        off_t size = i_blksize;

        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
            // First remove the prior block from the stats.
            if (wascommitted)
                m_committed -= prevsize;
            else
                m_uncommitted -= prevsize;

            // Add the new block to the stats.
            m_committed += size;

            // Update the entries.
            touch_entry(blkpath, mtime, size, true);
        }

        i_cmpl.bp_complete(i_keydata, i_keysize, i_argp);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bp_error(i_keydata, i_keysize, i_argp, ex);
    }
}

void
S3BlockStore::bs_refresh_start_async(uint64 i_rid,
                                     RefreshStartCompletion & i_cmpl,
                                     void const * i_argp)
    throw(InternalError)
{
    try
    {
        string rname = ridname(i_rid);
        string rpath = blockpath(rname);
 
        LOG(lgr, 6, m_instname << ' ' << "bs_refresh_start " << rname);

        time_t mtime = time(NULL);
        off_t size = 0;

        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            // Make sure it doesn't already exist.
            EntryHandle reh = new Entry(rpath, 0, 0);
            EntrySet::const_iterator pos = m_entries.find(reh);
            if (pos != m_entries.end())
                throwstream(NotUniqueError,
                            "refresh id " << i_rid << " already exists");

            // Create the refresh_id entry.
            touch_entry(rpath, mtime, size, true);
        }

        i_cmpl.rs_complete(i_rid, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rs_error(i_rid, i_argp, i_ex);
    }
}

void
S3BlockStore::bs_refresh_block_async(uint64 i_rid,
                                     void const * i_keydata,
                                     size_t i_keysize,
                                     RefreshBlockCompletion & i_cmpl,
                                     void const * i_argp)
    throw(InternalError,
          NotFoundError)
{
    // Perform the refresh on the in-memory data objects.
    //
    // Don't send anything to S3 here.

    string rname = ridname(i_rid);
    string rpath = blockpath(rname);

    // Make sure our refresh entry is here.
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
        EntryHandle reh = new Entry(rpath, 0, 0);
        EntrySet::const_iterator pos = m_entries.find(reh);
        if (pos == m_entries.end())
            throwstream(NotFoundError,
                        "refresh id " << i_rid << " not found"); 
    }

    string entry = entryname(i_keydata, i_keysize);
    string blkpath = blockpath(entry);

    LOG(lgr, 6, m_instname << ' '
        << "refreshing " << entry.substr(0,8) << "...");

    time_t mtime = time(NULL);
    off_t size = -1;

    bool found;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
        // Update the entries, don't create new ones.
        found = touch_entry(blkpath, mtime, size, false);
    }

    if (!found)
    {
        // Block is missing.
        i_cmpl.rb_missing(i_keydata, i_keysize, i_argp);
        return;
    }
    else
    {
        i_cmpl.rb_complete(i_keydata, i_keysize, i_argp);
        return;
    }
}
        
void
S3BlockStore::bs_refresh_finish_async(uint64 i_rid,
                                      RefreshFinishCompletion & i_cmpl,
                                      void const * i_argp)
    throw(InternalError)
{
    // Send a new metadata index of everything in the collection to
    // S3.
    //
    // Rename the metadata index over the old one.

    try
    {
        string rname = ridname(i_rid);
        string rpath = blockpath(rname);

        LOG(lgr, 6, m_instname << ' ' << "bs_refresh_finish " << rname);

        // Make sure our refresh entry is here.
        EntryHandle reh = new Entry(rpath, 0, 0);
        EntrySet::const_iterator pos = m_entries.find(reh);
        if (pos == m_entries.end())
            throwstream(NotFoundError, "refresh id " << i_rid << " not found"); 

        // Remove the MARK entry.
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            // Remove any pre-existing MARK entry.
            EntryHandle meh = new Entry("MARK", 0, 0);
            EntrySet::const_iterator pos = m_entries.find(meh);
            if (pos == m_entries.end())
            {
                // This is OK, won't be initially set ...
            }
            else
            {
                // Found it.
                meh = *pos;
 
                // Erase it from the entries set.
                m_entries.erase(pos);
 
                // Remove from current location in LRU list.
                m_lru.erase(meh->m_listpos);
            }

            // Find our RID entry.
            EntryHandle reh = new Entry(rpath, 0, 0);
            pos = m_entries.find(reh);
            if (pos == m_entries.end())
            {
                throwstream(InternalError, FILELINE
                            << "missing rid mark: " << rpath);
            }
            else
            {
                // Found it.
                reh = *pos;
 
                // Erase it from the entries set.
                m_entries.erase(pos);

                // Reinsert as the MARK, leave in the LRU list.
                reh->m_name = "MARK";
                m_entries.insert(reh);
            }

            // Add up all the committed memory.
            off_t committed = 0;
            off_t uncommitted = 0;
            bool saw_mark = false;
            for (EntryList::const_iterator it = m_lru.begin();
                 it != m_lru.end();
                 ++it)
            {
                EntryHandle const & eh = *it;

                if (saw_mark)
                {
                    uncommitted += eh->m_size;
                }
                else if (eh->m_name == "MARK")
                {
                    saw_mark = true;
                    m_mark = eh;
                }
                else
                {
                    committed += eh->m_size;
                }
            }

            m_committed = committed;
            m_uncommitted = uncommitted;
        }
        
        // Create an mdndx protocol buffer from our entries set.
        MDIndex mdndx;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);
            for (EntrySet::const_iterator it = m_entries.begin();
                 it != m_entries.end();
                 ++it)
            {
                MDEntry * mdep = mdndx.add_mdentry();
                mdep->set_name((*it)->m_name);
                mdep->set_mtime((*it)->m_tstamp);
            }
        }

        // Serialize it.
        string mdndxbuf;
        if (!mdndx.SerializeToString(&mdndxbuf))
            throwstream(InternalError, FILELINE
                        << "trouble serializing MDNDX");

        LOG(lgr, 6, m_instname << ' '
            << "writing MDNDX, size " << mdndxbuf.size());

        // Setup a bucket context.
        S3BucketContext buck;
        buck.bucketName = m_bucket_name.c_str();
        buck.protocol = m_protocol;
        buck.uriStyle = m_uri_style;
        buck.accessKeyId = m_access_key_id.c_str();
        buck.secretAccessKey = m_secret_access_key.c_str();

        // Update the MARKNAME file.
        MD5 md5sum(&mdndxbuf[0], mdndxbuf.size());
        S3PutProperties pp;
        ACE_OS::memset(&pp, '\0', sizeof(pp));
        pp.md5 = md5sum;

        for (unsigned i = 0; i < MAX_RETRIES; ++i)
        {
            if (i == MAX_RETRIES)
                throwstream(InternalError, FILELINE
                            << "too many retries");

            PutHandler ph((uint8 const *) &mdndxbuf[0], mdndxbuf.size());
            S3_put_object(&buck,
                          "MDNDX",
                          mdndxbuf.size(),
                          &pp,
                          NULL,
                          &put_tramp,
                          &ph);
            S3Status st = ph.wait();
            if (st == S3StatusOK)
                break;

            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, "mdndx update " << m_bucket_name
                << " ERROR: " << st << " RETRYING");
        }

        i_cmpl.rf_complete(i_rid, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rf_error(i_rid, i_argp, i_ex);
    }
}

void
S3BlockStore::bs_head_insert_async(SignedHeadEdge const & i_she,
                                   HeadEdgeInsertCompletion & i_cmpl,
                                   void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "insert " << i_she);

    write_head(i_she);

    m_lhng.insert_head(i_she);

    i_cmpl.hei_complete(i_she, i_argp);
}

void
S3BlockStore::bs_head_follow_async(HeadNode const & i_hn,
                                   HeadEdgeTraverseFunc & i_func,
                                   void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "follow " << i_hn);

    m_lhng.head_follow_async(i_hn, i_func, i_argp);
}

void
S3BlockStore::bs_head_furthest_async(HeadNode const & i_hn,
                                     HeadNodeTraverseFunc & i_func,
                                     void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "furthest " << i_hn);

    m_lhng.head_furthest_async(i_hn, i_func, i_argp);
}

void
S3BlockStore::bs_get_stats(StatSet & o_ss) const
    throw(InternalError)
{
    o_ss.set_name(m_instname);

    // FIXME - Add some stats here.
}

string 
S3BlockStore::blockpath(string const & i_entry)
{
    return string("BLOCKS/") + i_entry;
}                                

string 
S3BlockStore::edgepath(string const & i_edge)
{
    return string("EDGES/") + i_edge;
}                                

void
S3BlockStore::parse_params(StringSeq const & i_args,
                           S3Protocol & o_protocol,
                           S3UriStyle & o_uri_style,
                           string & o_access_key_id,
                           string & o_secret_access_key,
                           string & o_bucket_name)
{
    // For now just assign these
    o_protocol = S3ProtocolHTTP;
    // o_uri_style = S3UriStyleVirtualHost;
    o_uri_style = S3UriStylePath;

    string const KEY_ID = "--s3-access-key-id=";
    string const SECRET = "--s3-secret-access-key=";
    string const BUCKET = "--bucket=";

    for (unsigned i = 0; i < i_args.size(); ++i)
    {
        if (i_args[i].find(KEY_ID) == 0)
            o_access_key_id = i_args[i].substr(KEY_ID.length());

        else if (i_args[i].find(SECRET) == 0)
            o_secret_access_key = i_args[i].substr(SECRET.length());

        else if (i_args[i].find(BUCKET) == 0)
            o_bucket_name = i_args[i].substr(BUCKET.length());

        else
            throwstream(ValueError,
                        "unknown option S3BS parameter: " << i_args[i]);
    }

    if (o_access_key_id.empty())
        throwstream(ValueError, "S3BS parameter " << KEY_ID << " missing");

    if (o_secret_access_key.empty())
        throwstream(ValueError, "S3BS parameter " << SECRET << " missing");

    if (o_bucket_name.empty())
        throwstream(ValueError, "S3BS parameter " << BUCKET << " missing");

    // Perform one-time initialization.
    if (!c_s3inited)
    {
        S3_initialize(NULL, S3_INIT_ALL);
        c_s3inited = true;
    }
}

void
S3BlockStore::setup_params(StringSeq const & i_args)
{
    parse_params(i_args,
                 m_protocol,
                 m_uri_style,
                 m_access_key_id,
                 m_secret_access_key,
                 m_bucket_name);
}

string 
S3BlockStore::entryname(void const * i_keydata, size_t i_keysize) const
{
    return Base32::encode(i_keydata, i_keysize);
}

string 
S3BlockStore::ridname(uint64 i_rid) const
{
    ostringstream ostrm;
    ostrm << "RID-" << i_rid;
    return ostrm.str();
}

bool
S3BlockStore::touch_entry(std::string const & i_entry,
                          time_t i_mtime,
                          off_t i_size,
                          bool i_plsinsert)
{
    // IMPORTANT - This routine presumes you already hold the mutex.

    // Do we already have this entry in the table?
    EntryHandle eh = new Entry(i_entry, i_mtime, i_size);
    EntrySet::const_iterator pos = m_entries.find(eh);
    if (pos == m_entries.end())
    {
        // Not in the table.
        if (i_plsinsert)
        {
            // Not in the entry table yet, insert ...
            m_entries.insert(eh);
        }
        else
        {
            // Just return w/ bad status.
            return false;
        }
    }
    else
    {
        // Already in the entries table, use the existing entry.
        eh = *pos;

        // Remove from current location in LRU list.
        m_lru.erase(eh->m_listpos);

        // Update tstamp.
        eh->m_tstamp = i_mtime;
    }

    // Reinsert at the recent end of the LRU list.
    m_lru.push_front(eh);
    eh->m_listpos = m_lru.begin();

    return true;
}

void
S3BlockStore::purge_uncommitted()
{
    // IMPORTANT - This routine presumes you already hold the mutex.

    // Better have a list to work with.
    if (m_lru.empty())
        throwstream(InternalError, FILELINE
                    << "Shouldn't find LRU list empty here");

    // Need a MARK too.
    if (!m_mark)
        throwstream(InternalError, FILELINE
                    << "MARK needs to be set to purge uncommitted");

    // Find the oldest entry on the LRU list.
    EntryHandle eh = m_lru.back();

    // It needs to be older then the MARK (we have to grant equal here).
    if (eh->m_tstamp > m_mark->m_tstamp)
        throwstream(InternalError, FILELINE
                    << "LRU block on list is more recent then MARK");

    LOG(lgr, 6, m_instname << ' ' << "purge uncommitted: " << eh->m_name);

    // Remove from LRU list.
    m_lru.pop_back();

    {
        // Unlock the mutex while we run the S3 command.
        ACE_Reverse_Lock<ACE_Thread_Mutex> revmutex(m_s3bsmutex);
        ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > unguard(revmutex);

        // Setup a bucket context.
        S3BucketContext buck;
        buck.bucketName = m_bucket_name.c_str();
        buck.protocol = m_protocol;
        buck.uriStyle = m_uri_style;
        buck.accessKeyId = m_access_key_id.c_str();
        buck.secretAccessKey = m_secret_access_key.c_str();

        string blkpath = eh->m_name;
        ResponseHandler rh;
        S3_delete_object(&buck,
                         blkpath.c_str(),
                         NULL,
                         &rsp_tramp,
                         &rh);
        S3Status st = rh.wait();
        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);
    }

    // Update the accounting.
    m_uncommitted -= eh->m_size;
}

void
S3BlockStore::write_head(SignedHeadEdge const & i_she)
{
    string linebuf;
    i_she.SerializeToString(&linebuf);
    string encbuf = Base64::encode(linebuf.data(), linebuf.size());

    // Make up a unique name.
    Digest namedig(encbuf.data(), encbuf.size());
    string namestr = Base32::encode(namedig.data(), namedig.size());
    string edgekey = edgepath(namestr);

    LOG(lgr, 6, m_instname << ' ' << "write_head " << edgekey);

    // Generate the S3 checksum.
    MD5 md5sum(encbuf.data(), encbuf.size());
    S3PutProperties pp;
    ACE_OS::memset(&pp, '\0', sizeof(pp));
    pp.md5 = md5sum;

    // Setup a bucket context.
    S3BucketContext buck;
    buck.bucketName = m_bucket_name.c_str();
    buck.protocol = m_protocol;
    buck.uriStyle = m_uri_style;
    buck.accessKeyId = m_access_key_id.c_str();
    buck.secretAccessKey = m_secret_access_key.c_str();

    for (unsigned i = 0; i <= MAX_RETRIES; ++i)
    {
        PutHandler ph((uint8 const *) encbuf.data(), encbuf.size());
        S3_put_object(&buck,
                      edgekey.c_str(),
                      encbuf.size(),
                      &pp,
                      NULL,
                      &put_tramp,
                      &ph);
        S3Status st = ph.wait();
        switch (st)
        {
        case S3StatusOK:
            return;

        default:
            // Sigh ... these we retry a few times ...
            LOG(lgr, 4, m_instname << ' ' << "write_head " << m_bucket_name
                << " ERROR: " << st << " RETRYING");
            break;
        }
    }

    throwstream(InternalError, FILELINE << "too many retries");
}

// FIXME - Why do I have to copy this here from BlockStore.cpp?
ostream &
operator<<(ostream & ostrm, HeadNode const & i_nr)
{
    string pt1 = Base32::encode(i_nr.first.data(), i_nr.first.size());
    string pt2 = Base32::encode(i_nr.second.data(), i_nr.second.size());

    // Strip any trailing "====" off ...
    pt1 = pt1.substr(0, pt1.find_first_of('='));
    pt2 = pt2.substr(0, pt2.find_first_of('='));

    string::size_type sz1 = pt1.size();
    string::size_type sz2 = pt2.size();

    // How many characters of the FSID and NODEID should we display?
    static string::size_type const NFSID = 3;
    static string::size_type const NNDID = 5;

    // Use the right-justified substrings since the tests sometimes
    // only differ in the right positions.  Real digest based refs
    // will differ in all positions.
    //
    string::size_type off1 = sz1 > NFSID ? sz1 - NFSID : 0;
    string::size_type off2 = sz2 > NNDID ? sz2 - NNDID : 0;

    ostrm << pt1.substr(off1) << ':' << pt2.substr(off2);

    return ostrm;
}

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
