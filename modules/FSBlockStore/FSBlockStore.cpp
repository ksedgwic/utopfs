#include <fstream>
#include <sstream>
#include <string>

#include <ace/OS_NS_sys_stat.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/Dirent.h>

#include "Base32.h"
#include "Base64.h"
#include "BlockStoreFactory.h"
#include "Log.h"

#include "FSBlockStore.h"
#include "fsbslog.h"

using namespace std;
using namespace utp;

namespace FSBS {

#if defined(WIN32)

#define S_ISDIR(stmode)  \
    ((stmode & S_IFMT ) == S_IFDIR)

#define S_ISREG(stmode)  \
    ((stmode & S_IFMT ) == S_IFREG)

// This function is for temporary use only, it ignores second
// parameter and sets current time only. This should be replaced
// when high resolution timer is implemented.
// TODO:implement high resolution time on windows.

int utimes(const char *filename, const struct timeval times[2])
{
    ACE_HANDLE fh = ACE_OS::open(filename, O_RDWR | O_CREAT);

    if (fh == ACE_INVALID_HANDLE)
    {
        LOG(lgr, 4, "utimes " << "trouble opening file to touch" << ' ' << filename);
        return -1;
    }

    FILETIME nowft;
    GetSystemTimeAsFileTime (&nowft);

    //SetFileTime function returns zero if fails and non zero of succeeds.
    int ret = !SetFileTime(fh, (LPFILETIME) NULL, &nowft, &nowft);

    ACE_OS::close(fh);
    return ret;
}

#endif

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

void
FSBlockStore::destroy(StringSeq const & i_args)
{
    // This is painful, but at least we can make it safer then the old
    // "rm -rf" approach.
    //
    // We loop over all the elements of the blockstore and remove them.
    //
    // Mostly we ignore errors, but not if the top level isn't what we
    // think it is.

    string const & path = i_args[0];

    LOG(lgr, 4, "destroy " << path);
    
    ACE_stat sb;

    // Make sure the top level is a directory.
    if (ACE_OS::stat(path.c_str(), &sb) != 0)
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: top dir \""
                    << path << "\" does not exist");
    
    if (! S_ISDIR(sb.st_mode))
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: top dir \""
                    << path << "\" not directory");

    // Make sure the size file exists.
    string sizepath = path + "/SIZE";
    if (ACE_OS::stat(sizepath.c_str(), &sb))
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: size file \""
                    << sizepath << "\" does not exist");
    
    if (! S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: size file \""
                    << sizepath << "\" not file");

    // Unlink the SIZE file
    if (ACE_OS::unlink(sizepath.c_str()))
        throwstream(InternalError, FILELINE
                    << "unlinking file " << sizepath.c_str() << " failed: "
                    << ACE_OS::strerror(errno));

    // Unlink the HEADS file
    string headspath = path + "/HEADS";
    if (ACE_OS::unlink(headspath.c_str()))
        throwstream(InternalError, FILELINE
                    << "unlinking file " << headspath.c_str() << " failed: "
                    << ACE_OS::strerror(errno));


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
        if (ACE_OS::unlink(blkpath.c_str()))
            throwstream(InternalError, FILELINE
                    << "unlinking file " << blkpath.c_str() << " failed: "
                    << ACE_OS::strerror(errno));

    }

    // Remove the blocks subdir.
    if (ACE_OS::rmdir(blockspath.c_str()))
        throwstream(InternalError, FILELINE
                    << "FSBlockStore::destroy failed: "
                    << ACE_OS::strerror(errno));

    // Remove the path.
    if (ACE_OS::rmdir(path.c_str()))
        throwstream(InternalError, FILELINE
                    << "FSBlockStore::destroy failed: "
                    << ACE_OS::strerror(errno));
}

FSBlockStore::FSBlockStore(string const & i_instname)
    : m_instname(i_instname)
    , m_size(0)
    , m_committed(0)
    , m_uncommitted(0)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");
}

FSBlockStore::~FSBlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)

    // Close the HEADS stream.
    if (m_headsstrm.is_open())
        m_headsstrm.close();
}

string const &
FSBlockStore::bs_instname() const
{
    return m_instname;
}

void
FSBlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    string const & path = i_args[0];

    LOG(lgr, 4, m_instname << ' ' << "bs_create " << i_size << ' ' << path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

    struct stat statbuff;    
    if (stat(path.c_str(),&statbuff) == 0) {
         throwstream(NotUniqueError, FILELINE
                << "Cannot create file block store at '" << path
                     << "'. File or directory already exists.");   
    }

    m_size = i_size;
    m_uncommitted = 0;
    m_committed = 0;

    // Make the parent directory.
    m_rootpath = path;
    if (ACE_OS::mkdir(m_rootpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        throwstream(InternalError, FILELINE
                    << "mkdir " << m_rootpath << "failed: "
                    << ACE_OS::strerror(errno));

    // Make the BLOCKS subdir.
    m_blockspath = m_rootpath + "/BLOCKS";
    if (ACE_OS::mkdir(m_blockspath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        throwstream(InternalError, FILELINE
                    << "mkdir " << m_blockspath << "failed: "
                    << ACE_OS::strerror(errno));

    // Store the size in the root.
    string szpath = m_rootpath + "/SIZE";
    ofstream szstrm(szpath.c_str());
    szstrm << m_size << endl;
    szstrm.close();

    // Open the HEADS output stream.
    m_headsstrm.open(headspath().c_str(), ofstream::out | ofstream::app);
    if (!m_headsstrm.good())
        throwstream(InternalError, FILELINE
                    << "Trouble opening " << headspath() << ": "
                    << ACE_OS::strerror(errno));
}

void
FSBlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    if (i_args.empty())
        throwstream(ValueError, "missing blockstore path argument");

    string const & path = i_args[0];

    LOG(lgr, 4, m_instname << ' ' << "bs_open " << path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

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

    // Read all of the existing SignedHeadEdges
    ifstream shestrm(headspath().c_str());
    string linebuf;
    while (getline(shestrm, linebuf))
    {
        SignedHeadEdge she;
        string data = Base64::decode(linebuf);
        int ok = she.ParseFromString(data);
        if (!ok)
            throwstream(InternalError, FILELINE
                        << " SignedHeadEdge deserialize failed");
        m_lhng.insert_head(she);
    }
    
    // Open the HEADS output stream.
    m_headsstrm.open(headspath().c_str(), ofstream::out | ofstream::app);
    if (!m_headsstrm.good())
        throwstream(InternalError, FILELINE
                    << "Trouble opening " << headspath() << ": "
                    << ACE_OS::strerror(errno));
}

void
FSBlockStore::bs_close()
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

    // Close the HEADS stream.
    if (m_headsstrm.is_open())
        m_headsstrm.close();
}

void
FSBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "bs_stat");

    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

    o_stat.bss_size = m_size;
    o_stat.bss_free = m_size - m_committed;
}

void
FSBlockStore::bs_sync()
    throw(InternalError)
{
	//always synced to disk
}

void
 FSBlockStore::bs_block_get_async(void const * i_keydata,
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
        int bytes_read;
        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

            if (i_keysize == 0)
                throwstream(NotFoundError,
                            "empty key is always not found");

            string entry = entryname(i_keydata, i_keysize);
            string blkpath = blockpath(entry);

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
                                << "FSBlockStore::bs_get_block: "
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

#if defined (WIN32)
// Specifies no sharing flags.
            int perms = ACE_DEFAULT_OPEN_PERMS;
#else
            int perms = S_IRUSR;
#endif
            ACE_HANDLE fh = ACE_OS::open(blkpath.c_str(), O_RDONLY, perms);
            bytes_read = ACE_OS::read(fh, o_buffdata, i_buffsize);
            ACE_OS::close(fh);

            if (bytes_read == -1) {
                throwstream(InternalError, FILELINE
                            << "FSBlockStore::bs_get_block: read failed: "
                            << ACE_OS::strerror(errno));
            }

        
            if (bytes_read != statbuff.st_size) {
                throwstream(InternalError, FILELINE
                            << "FSBlockStore::expected to get "
                            << statbuff.st_size
                            << " bytes, but got " << bytes_read << " bytes");
            }

            // Release the mutex before the completion function.
        }

        i_cmpl.bg_complete(i_keydata, i_keysize, i_argp, bytes_read);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bg_error(i_keydata, i_keysize, i_argp, ex);
    }
}

void
FSBlockStore::bs_block_put_async(void const * i_keydata,
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
        LOG(lgr, 6, m_instname << ' ' << "bs_block_put");

        {
            ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

            string entry = entryname(i_keydata, i_keysize);
            string blkpath = blockpath(entry);
    
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

#if defined (WIN32)
// Specifies no sharing flags.
            int perms = ACE_DEFAULT_OPEN_PERMS;
#else
            int perms = S_IRUSR | S_IWUSR;
#endif

            ACE_HANDLE fh = ACE_OS::open(blkpath.c_str(),
                          O_CREAT | O_TRUNC | O_WRONLY, perms);    
            if (fh == ACE_INVALID_HANDLE)
                throwstream(InternalError, FILELINE
                            << "FSBlockStore::bs_block_put: open failed on "
                            << blkpath << ": " << ACE_OS::strerror(errno));
    
            int bytes_written = ACE_OS::write(fh, i_blkdata, i_blksize);
            int write_errno = errno;
            ACE_OS::close(fh);
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

            // Release the mutex before the completion function.
        }

        i_cmpl.bp_complete(i_keydata, i_keysize, i_argp);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bp_error(i_keydata, i_keysize, i_argp, ex);
    }
}

void
FSBlockStore::bs_refresh_start_async(uint64 i_rid,
                                     RefreshStartCompletion & i_cmpl,
                                     void const * i_argp)
    throw(InternalError)
{
    try
    {
        string rname = ridname(i_rid);
        string rpath = blockpath(rname);
 
        LOG(lgr, 6, m_instname << ' ' << "bs_refresh_start " << rname);

        ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

        // Does the refresh ID already exist?
        ACE_stat sb;
        int rv = ACE_OS::stat(rpath.c_str(), &sb);
        if (rv != -1)
            throwstream(NotUniqueError,
                        "refresh id " << i_rid << " already exists");

#if defined (WIN32)
// Specifies no sharing flags.
            int perms = ACE_DEFAULT_OPEN_PERMS;
#else
            int perms = S_IRUSR;
#endif

        // Create the refresh id mark.
        ACE_HANDLE fh = ACE_OS::open(rpath.c_str(), O_RDWR | O_CREAT, perms);

        if (fh == ACE_INVALID_HANDLE)
            throwstream(InternalError, FILELINE
                        << "mknod " << rpath << " failed: "
                        << ACE_OS::strerror(errno));

        ACE_OS::close(fh);

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

        i_cmpl.rs_complete(i_rid, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rs_error(i_rid, i_argp, i_ex);
    }
}

void
FSBlockStore::bs_refresh_block_async(uint64 i_rid,
                                     void const * i_keydata,
                                     size_t i_keysize,
                                     RefreshBlockCompletion & i_cmpl,
                                     void const * i_argp)
    throw(InternalError,
          NotFoundError)
{
    string rname = ridname(i_rid);
    string rpath = blockpath(rname);

    string entry = entryname(i_keydata, i_keysize);
    string blkpath = blockpath(entry);

    bool ismissing = false;

    LOG(lgr, 6, m_instname << ' '
        << "bs_refresh_block_async " << rname << ' ' << entry);

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

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
    }

    if (ismissing)
        i_cmpl.rb_missing(i_keydata, i_keysize, i_argp);
    else
        i_cmpl.rb_complete(i_keydata, i_keysize, i_argp);
}
        
void
FSBlockStore::bs_refresh_finish_async(uint64 i_rid,
                                      RefreshFinishCompletion & i_cmpl,
                                      void const * i_argp)
    throw(InternalError)
{
    try
    {
        string rname = ridname(i_rid);
        string rpath = blockpath(rname);

        LOG(lgr, 6, m_instname << ' ' << "bs_refresh_finish " << rname);

        ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

        // Does the refresh ID exist?
        ACE_stat sb;
        int rv = ACE_OS::stat(rpath.c_str(), &sb);
        if (rv != 0 || !S_ISREG(sb.st_mode))
            throwstream(NotFoundError,
                        "refresh id " << i_rid << " not found");

        // Rename the refresh tag to the mark name.
        string mname = markname();
        string mpath = blockpath(mname);
        if (ACE_OS::rename(rpath.c_str(), mpath.c_str()) != 0)
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
            throwstream(InternalError, FILELINE
                        << "Missing RID entry " << rname);
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
        for (EntryList::const_iterator it = m_lru.begin();
             it != m_lru.end();
             ++it)
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

        i_cmpl.rf_complete(i_rid, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rf_error(i_rid, i_argp, i_ex);
    }
}

void
FSBlockStore::bs_head_insert_async(SignedHeadEdge const & i_she,
                                   HeadEdgeInsertCompletion & i_cmpl,
                                   void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "insert " << i_she);

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

        write_head(i_she);
    }

    m_lhng.insert_head(i_she);

    i_cmpl.hei_complete(i_she, i_argp);
}

void
FSBlockStore::bs_head_follow_async(HeadNode const & i_hn,
                                   HeadEdgeTraverseFunc & i_func,
                                   void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "follow " << i_hn);

    m_lhng.head_follow_async(i_hn, i_func, i_argp);
}

void
FSBlockStore::bs_head_furthest_async(HeadNode const & i_hn,
                                     HeadNodeTraverseFunc & i_func,
                                     void const * i_argp)
    throw(InternalError)
{
    LOG(lgr, 6, m_instname << ' ' << "furthest " << i_hn);

    m_lhng.head_furthest_async(i_hn, i_func, i_argp);
}

void
FSBlockStore::bs_get_stats(StatSet & o_ss) const
    throw(InternalError)
{
    o_ss.set_name(m_instname);

    // FIXME - Add some stats here.
}

string 
FSBlockStore::entryname(void const * i_keydata, size_t i_keysize) const
{
    return Base32::encode(i_keydata, i_keysize);
}

string 
FSBlockStore::ridname(uint64 i_rid) const
{
    ostringstream ostrm;
    ostrm << "RID-" << i_rid;
    return ostrm.str();
}                                

string 
FSBlockStore::blockpath(string const & i_entry) const
{
    return m_blockspath + '/' + i_entry;
}                                

string 
FSBlockStore::headspath() const
{
    return m_rootpath + "/HEADS";
}                                

void
FSBlockStore::touch_entry(string const & i_entry,
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
FSBlockStore::purge_uncommitted()
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

    // Remove from storage.
    string blkpath = blockpath(eh->m_name);
    if (ACE_OS::unlink(blkpath.c_str()) != 0)
        throwstream(InternalError, FILELINE
                    << "unlink " << blkpath << " failed: "
                    << ACE_OS::strerror(errno));

    // Update the accounting.
    m_uncommitted -= eh->m_size;
}

void
FSBlockStore::write_head(SignedHeadEdge const & i_she)
{
    string linebuf;
    i_she.SerializeToString(&linebuf);
    m_headsstrm << Base64::encode(linebuf.data(), linebuf.size()) << endl;
}

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
