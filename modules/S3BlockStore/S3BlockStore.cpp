#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>
#include <ace/Dirent.h>

#include "Log.h"

#include "S3BlockStore.h"
#include "s3bslog.h"

#include "Base32.h"
#include "MD5.h"

using namespace std;
using namespace utp;

namespace S3BS {

std::ostream & operator<<(std::ostream & ostrm, S3Status status)
{
    ostrm << S3_get_status_name(status);
    return ostrm;
}
    
class ResponseHandler
{
public:
    ResponseHandler()
        : m_s3rhcond(m_s3rhmutex)
        , m_complete(false)
    {
    }

    virtual S3Status rh_properties(S3ResponseProperties const * properties)
    {
        return S3StatusOK;
    }

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails)
    {
        LOG(lgr, 6, "rh_complete: " << status);

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
        LOG(lgr, 6, "PutHandler CTOR " << i_size);
    }

    virtual int ph_objdata(int i_buffsz, char * o_buffer)
    {
        size_t sz = min(size_t(i_buffsz), m_size);

        LOG(lgr, 6, "ph_objdata: " << sz);

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
    KeyListHandler(StringSeq & o_keys) : m_keys(o_keys) {}

    virtual S3Status lh_item(int i_istrunc,
                             char const * i_next_marker,
                             int i_contents_count,
                             S3ListBucketContent const * i_contents,
                             int i_common_prefixes_count,
                             char const ** i_common_prefixes)
    {
        if (i_istrunc)
            throwstream(InternalError, FILELINE
                        << "truncated lists make me sad");

        if (i_common_prefixes_count)
            throwstream(InternalError, FILELINE
                        << "common prefixes make me sad");

        for (int i = 0; i < i_contents_count; ++i)
        {
            S3ListBucketContent const * cp = &i_contents[i];
            LOG(lgr, 7, "lh_item " << cp->size << ' ' << cp->key);
            m_keys.push_back(cp->key);
        }

        return S3StatusOK;
    }
    
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
    KeyListHandler klh(keys);
    S3_list_bucket(&buck,
                   NULL,
                   NULL,
                   NULL,
                   INT_MAX,
                   NULL,
                   &lst_tramp,
                   &klh);
    st = klh.wait();
    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);

    // Delete all of the keys.
    for (unsigned i = 0; i < keys.size(); ++i)
    {
        ResponseHandler rh;
        S3_delete_object(&buck,
                         keys[i].c_str(),
                         NULL,
                         &rsp_tramp,
                         &rh);
        st = rh.wait();
        if (st != S3StatusOK)
            throwstream(InternalError, FILELINE
                        << "Unexpected S3 error: " << st);
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

    // Re-init the s3 context; it appears buckets don't appear
    // to go away until we re-initialize ... sigh.
    //
    S3_deinitialize();
    S3_initialize(NULL, S3_INIT_ALL);

    // Unfortunately we must poll until it actually goes away ...
    for (unsigned i = 0; true; ++i)
    {
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

#if 0    
    ACE_stat sb;

    // Make sure the top level is a directory.
    if (ACE_OS::stat(path.c_str(), &sb) != 0)
        throwstream(NotFoundError,
                    "S3BlockStore::destroy: top dir \""
                    << path << "\" does not exist");
    
    if (! S_ISDIR(sb.st_mode))
        throwstream(NotFoundError,
                    "S3BlockStore::destroy: top dir \""
                    << path << "\" not directory");

    // Make sure the size file exists.
    string sizepath = path + "/SIZE";
    if (ACE_OS::stat(sizepath.c_str(), &sb) != 0)
        throwstream(NotFoundError,
                    "S3BlockStore::destroy: size file \""
                    << sizepath << "\" does not exist");
    
    if (! S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "S3BlockStore::destroy: size file \""
                    << sizepath << "\" not file");

    // Unlink the SIZE file
    unlink(sizepath.c_str());

    // Read all of the existing blocks.
    string blockspath = path + "/BLOCKS";
    ACE_Dirent dir;
    if (dir.open(blockspath.c_str()) == -1)
        throwstream(InternalError, FILELINE
                    << "dir open " << path << " failed: "
                    << ACE_OS::strerror(errno));
    for (ACE_DIRENT * dep = dir.read(); dep; dep = dir.read())
    {
        string entry = dep->d_name;

        // Skip '.' and '..'.
        if (entry == "." || entry == "..")
            continue;

        // Remove the block.
        string blkpath = blockspath + '/' + entry;
        unlink(blkpath.c_str());
    }

    // Remove the blocks subdir.
    rmdir(blockspath.c_str());

    // Remove the path.
    if (rmdir(path.c_str()))
        throwstream(InternalError, FILELINE
                    << "S3BlockStore::destroy failed: "
                    << ACE_OS::strerror(errno));
#endif
}

S3BlockStore::S3BlockStore()
    : m_size(0)
    , m_committed(0)
    , m_uncommitted(0)
    , m_blockspath("BLOCKS")
{
    LOG(lgr, 4, "CTOR");
}

S3BlockStore::~S3BlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
S3BlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    m_size = i_size;
    m_uncommitted = 0;
    m_committed = 0;

    setup_params(i_args);

    LOG(lgr, 4, "bs_create " << i_size << ' ' << m_bucket_name);

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

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
    LOG(lgr, 6, "md5: \"" << md5sum << "\"");

    S3PutProperties pp;
    ACE_OS::memset(&pp, '\0', sizeof(pp));
    pp.md5 = md5sum;

    PutHandler ph((uint8 const *) obj.data(), obj.size());

    S3_put_object(&buck,
                  "SIZE",
                  obj.size(),
                  &pp,
                  NULL,
                  &put_tramp,
                  &ph);

    st = ph.wait();

    if (st != S3StatusOK)
        throwstream(InternalError, FILELINE
                    << "Unexpected S3 error: " << st);
}

void
S3BlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError)
{
    setup_params(i_args);

    LOG(lgr, 4, "bs_open " << m_bucket_name);

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

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
    LOG(lgr, 4, "bs_open size=" << m_size);

#if 0    
    string const & path = i_args[0];

    LOG(lgr, 4, "bs_open " << path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

    ACE_stat sb;
    
    if (ACE_OS::stat(path.c_str(), &sb) != 0)
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << path
                    << "'. Directory does not exist.");    
    
    if (! S_ISDIR(sb.st_mode))
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << path
                    << "'. Path is not a directory.");

    m_rootpath = path;
    m_blockspath = m_rootpath + "/BLOCKS";

    // Figure out the size.
    string szpath = m_rootpath + "/SIZE";
    ifstream szstrm(szpath.c_str());
    szstrm >> m_size;
    szstrm.close();

    // Read all of the existing blocks.
    EntryTimeSet ets;
    ACE_Dirent dir;
    if (dir.open(m_blockspath.c_str()) == -1)
        throwstream(InternalError, FILELINE
                    << "dir open " << path << " failed: "
                    << ACE_OS::strerror(errno));
    for (ACE_DIRENT * dep = dir.read(); dep; dep = dir.read())
    {
        string entry = dep->d_name;

        // Skip '.' and '..'.
        if (entry == "." || entry == "..")
            continue;

        // Figure out the timestamp.
        string blkpath = m_blockspath + '/' + entry;
        if (ACE_OS::stat(blkpath.c_str(), &sb) != 0)
            throwstream(InternalError, FILELINE
                        << "trouble w/ stat of " << blkpath << ": "
                        << ACE_OS::strerror(errno));
        time_t mtime = sb.st_mtime;
        size_t size = sb.st_size;

        // Insert into the entries and time-sorted entries sets.
        EntryHandle eh = new Entry(entry, mtime, size);
        m_entries.insert(eh);
        ets.insert(eh);
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
            if (eh->m_name == markname())
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
#endif
}

void
S3BlockStore::bs_close()
    throw(InternalError)
{
    LOG(lgr, 4, "bs_close");
}

void
S3BlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    LOG(lgr, 6, "bs_stat");

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

    o_stat.bss_size = m_size;
    o_stat.bss_free = m_size - m_committed;
}

void
S3BlockStore::bs_get_block_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void * o_buffdata,
                                 size_t i_buffsize,
                                 BlockGetCompletion & i_cmpl)
    throw(InternalError,
          ValueError)
{
    try
    {
        int bytes_read;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            string entry = entryname(i_keydata, i_keysize);
            string blkpath = blockpath(entry);

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

            bytes_read = gh.size();
#if 0
            ACE_stat statbuff;

            int rv = ACE_OS::stat(blkpath.c_str(), &statbuff);
            if (rv == -1)
            {
                if (errno == ENOENT)
                    throwstream(NotFoundError,
                                Base32::encode(i_keydata, i_keysize)
                                << ": not found");
                else
                    throwstream(InternalError, FILELINE
                                << "S3BlockStore::bs_get_block: "
                                << Base32::encode(i_keydata, i_keysize)
                                << ": error: " << ACE_OS::strerror(errno));
            }

            if (statbuff.st_size > off_t(i_buffsize))
            {
                throwstream(ValueError, FILELINE
                            << "buffer overflow: "
                            << "buffer " << i_buffsize << ", "
                            << "data " << statbuff.st_size);
            }

            int fd = open(blkpath.c_str(), O_RDONLY, S_IRUSR);
            bytes_read = read(fd, o_buffdata, i_buffsize);
            close(fd);

            if (bytes_read == -1) {
                throwstream(InternalError, FILELINE
                            << "S3BlockStore::bs_get_block: read failed: "
                            << strerror(errno));
            }

        
            if (bytes_read != statbuff.st_size) {
                throwstream(InternalError, FILELINE
                            << "S3BlockStore::expected to get "
                            << statbuff.st_size
                            << " bytes, but got " << bytes_read << " bytes");
            }
#endif

            // Release the mutex before the completion function.
        }

        i_cmpl.bg_complete(i_keydata, i_keysize, bytes_read);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bg_error(i_keydata, i_keysize, ex);
    }
}

void
S3BlockStore::bs_put_block_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void const * i_blkdata,
                                 size_t i_blksize,
                                 BlockPutCompletion & i_cmpl)
    throw(InternalError,
          ValueError)
{
    try
    {
        LOG(lgr, 6, "bs_put_block");

        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

            string entry = entryname(i_keydata, i_keysize);
            string blkpath = blockpath(entry);

            // Setup a bucket context.
            S3BucketContext buck;
            buck.bucketName = m_bucket_name.c_str();
            buck.protocol = m_protocol;
            buck.uriStyle = m_uri_style;
            buck.accessKeyId = m_access_key_id.c_str();
            buck.secretAccessKey = m_secret_access_key.c_str();

            MD5 md5sum(i_blkdata, i_blksize);
            LOG(lgr, 6, "md5: \"" << md5sum << "\"");

            S3PutProperties pp;
            ACE_OS::memset(&pp, '\0', sizeof(pp));
            pp.md5 = md5sum;

            PutHandler ph((uint8 const *) i_blkdata, i_blksize);

            S3_put_object(&buck,
                          blkpath.c_str(),
                          i_blksize,
                          &pp,
                          NULL,
                          &put_tramp,
                          &ph);

            S3Status st = ph.wait();

            if (st != S3StatusOK)
                throwstream(InternalError, FILELINE
                            << "Unexpected S3 error: " << st);
            

#if 0    
            // Need to stat the block first so we can keep the size accounting
            // straight ...
            //
            off_t prevsize = 0;
            bool wascommitted = true;
            ACE_stat sb;
            int rv = ACE_OS::stat(blkpath.c_str(), &sb);
            if (rv == 0)
            {
                prevsize = sb.st_size;

                // Is this block older then the MARK?
                if (m_mark && m_mark->m_tstamp > sb.st_mtime)
                    wascommitted = false;
            }

            off_t prevcommited = 0;
            if (wascommitted)
                prevcommited = prevsize;

            // How much space will be available for this block?
            off_t avail = m_size - m_committed + prevcommited;

            if (off_t(i_blksize) > avail)
                throwstream(NoSpaceError,
                            "insufficent space: "
                            << avail << " bytes avail, needed " << i_blksize);

            // Do we need to remove uncommitted blocks to make room for this
            // block?
            while (m_committed + off_t(i_blksize) +
                   m_uncommitted - prevcommited > m_size)
                purge_uncommitted();

            int fd = open(blkpath.c_str(),
                          O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
            if (fd == -1)
                throwstream(InternalError, FILELINE
                            << "S3BlockStore::bs_put_block: open failed on "
                            << blkpath << ": " << strerror(errno));
    
            int bytes_written = write(fd, i_blkdata, i_blksize);
            int write_errno = errno;
            close(fd);
            if (bytes_written == -1)
                throwstream(InternalError, FILELINE
                            << "write of " << blkpath << " failed: "
                            << ACE_OS::strerror(write_errno));

            // Stat the file we just wrote so we can use the exact tstamp
            // and size the filesystem sees.
            rv = ACE_OS::stat(blkpath.c_str(), &sb);
            if (rv == -1)
                throwstream(InternalError, FILELINE
                            << "stat " << blkpath << " failed: "
                            << ACE_OS::strerror(errno));
            time_t mtime = sb.st_mtime;
            off_t size = sb.st_size;

            // First remove the prior block from the stats.
            if (wascommitted)
                m_committed -= prevsize;
            else
                m_uncommitted -= prevsize;

            // Add the new block to the stats.
            m_committed += size;

            // Update the entries.
            touch_entry(entry, mtime, size);
#endif

            // Release the mutex before the completion function.
        }

        i_cmpl.bp_complete(i_keydata, i_keysize);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bp_error(i_keydata, i_keysize, ex);
    }
}

void
S3BlockStore::bs_refresh_start(uint64 i_rid)
    throw(InternalError,
          NotUniqueError)
{
    string rname = ridname(i_rid);
    string rpath = blockpath(rname);
 
    LOG(lgr, 6, "bs_refresh_start " << rname);

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

#if 0
    // Does the refresh ID already exist?
    ACE_stat sb;
    int rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv != -1)
        throwstream(NotUniqueError,
                    "refresh id " << i_rid << " already exists");

    // Create the refresh id mark.
    if (mknod(rpath.c_str(), S_IFREG, 0) != 0)
        throwstream(InternalError, FILELINE
                    << "mknod " << rpath << " failed: "
                    << ACE_OS::strerror(errno));

    // Stat the file we just wrote so we can use the exact tstamp
    // and size the filesystem sees.
    rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv == -1)
        throwstream(InternalError, FILELINE
                    << "stat " << rpath << " failed: "
                    << ACE_OS::strerror(errno));
    time_t mtime = sb.st_mtime;
    off_t size = sb.st_size;

    // Create the refresh_id entry.
    touch_entry(rname, mtime, size);
#endif
}

void
S3BlockStore::bs_refresh_block_async(uint64 i_rid,
                                     void const * i_keydata,
                                     size_t i_keysize,
                                     BlockRefreshCompletion & i_cmpl)
    throw(InternalError,
          NotFoundError)
{
    string rname = ridname(i_rid);
    string rpath = blockpath(rname);

    string entry = entryname(i_keydata, i_keysize);
    string blkpath = blockpath(entry);

    bool ismissing = false;

    LOG(lgr, 6, "bs_refresh_block_async " << rname << ' ' << entry);

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

#if 0
        // Does the refresh ID exist?
        ACE_stat sb;
        int rv = ACE_OS::stat(rpath.c_str(), &sb);
        if (rv != 0 || !S_ISREG(sb.st_mode))
            throwstream(NotFoundError,
                        "refresh id " << i_rid << " not found");

        // If the block doesn't exist add it to the missing list.
        rv = ACE_OS::stat(blkpath.c_str(), &sb);
        if (rv != 0 || !S_ISREG(sb.st_mode))
        {
            ismissing = true;
        }
        else
        {
            // Touch the block.
            rv = utimes(blkpath.c_str(), NULL);
            if (rv != 0)
                throwstream(InternalError, FILELINE
                            << "trouble touching \"" << blkpath
                            << "\": " << ACE_OS::strerror(errno));

            // Stat the file we just wrote so we can use the exact tstamp
            // and size the filesystem sees.
            rv = ACE_OS::stat(blkpath.c_str(), &sb);
            if (rv == -1)
                throwstream(InternalError, FILELINE
                            << "stat " << blkpath << " failed: "
                            << ACE_OS::strerror(errno));
            time_t mtime = sb.st_mtime;
            off_t size = sb.st_size;

            // Update the entries.
            touch_entry(entry, mtime, size);
        }
#endif
    }

    if (ismissing)
        i_cmpl.br_missing(i_keydata, i_keysize);
    else
        i_cmpl.br_complete(i_keydata, i_keysize);
}
        
void
S3BlockStore::bs_refresh_finish(uint64 i_rid)
    throw(InternalError,
          NotFoundError)
{
    string rname = ridname(i_rid);
    string rpath = blockpath(rname);

    LOG(lgr, 6, "bs_refresh_finish " << rname);

    ACE_Guard<ACE_Thread_Mutex> guard(m_s3bsmutex);

#if 0
    // Does the refresh ID exist?
    ACE_stat sb;
    int rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv != 0 || !S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "refresh id " << i_rid << " not found");

    // Rename the refresh tag to the mark name.
    string mname = markname();
    string mpath = blockpath(mname);
    if (rename(rpath.c_str(), mpath.c_str()) != 0)
        throwstream(InternalError,
                    "rename " << rpath << ' ' << mpath << " failed:"
                    << ACE_OS::strerror(errno));

    // Remove the MARK entry.
    EntryHandle meh = new Entry(mname, 0, 0);
    EntrySet::const_iterator pos = m_entries.find(meh);
    if (pos == m_entries.end())
    {
        // Not in the entry table yet, no action needed ...
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

    // Find the Refresh token.
    EntryHandle reh = new Entry(rname, 0, 0);
    pos = m_entries.find(reh);
    if (pos == m_entries.end())
    {
        // Not in the entry table yet, very bad!
        throwstream(InternalError, FILELINE << "Missing RID entry " << rname);
    }
    else
    {
        // Found it.
        reh = *pos;

        // Erase it from the entries set.
        m_entries.erase(pos);

        // Change it's name to the MARK.
        reh->m_name = markname();

        // Reinsert, leave in current spot in the LRU list with it's
        // current tstamp.
        //
        m_entries.insert(reh);

        m_mark = reh;
    }

    // Add up all the committed memory.
    off_t committed = 0;
    off_t uncommitted = 0;
    bool saw_mark = false;
    for (EntryList::const_iterator it = m_lru.begin(); it != m_lru.end(); ++it)
    {
        EntryHandle const & eh = *it;

        if (saw_mark)
        {
            uncommitted += eh->m_size;
        }
        else if (eh->m_name == markname())
        {
            saw_mark = true;
        }
        else
        {
            committed += eh->m_size;
        }
    }

    m_committed = committed;
    m_uncommitted = uncommitted;
#endif
}

void
S3BlockStore::bs_sync()
    throw(InternalError)
{
	//always synced to disk
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
    o_protocol = S3ProtocolHTTPS;
    o_uri_style = S3UriStyleVirtualHost;

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
    ostrm << "RID:" << i_rid;
    return ostrm.str();
}                                

string 
S3BlockStore::blockpath(string const & i_entry) const
{
    return m_blockspath + '/' + i_entry;
}                                

void
S3BlockStore::touch_entry(std::string const & i_entry,
                          time_t i_mtime,
                          off_t i_size)
{
    // IMPORTANT - This routine presumes you already hold the mutex.

    // Do we already have this entry in the table?
    EntryHandle eh = new Entry(i_entry, i_mtime, i_size);
    EntrySet::const_iterator pos = m_entries.find(eh);
    if (pos == m_entries.end())
    {
        // Not in the entry table yet, insert ...
        m_entries.insert(eh);
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

    LOG(lgr, 6, "purge uncommitted: " << eh->m_name);

    // Remove from LRU list.
    m_lru.pop_back();

    // Remove from storage.
    string blkpath = blockpath(eh->m_name);
    if (unlink(blkpath.c_str()) != 0)
        throwstream(InternalError, FILELINE
                    << "unlink " << blkpath << " failed: "
                    << ACE_OS::strerror(errno));

    // Update the accounting.
    m_uncommitted -= eh->m_size;
}

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
