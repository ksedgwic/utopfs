#include <fstream>
#include <sstream>
#include <string>

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

ostream &
operator<<(ostream & ostrm, NodeRef const & i_nr)
{
    string pt1 = Base32::encode(i_nr.first.data(), i_nr.first.size());
    string pt2 = Base32::encode(i_nr.second.data(), i_nr.second.size());

    // Use the ends because tests vary on one end or the other ...
    ostrm << pt1.substr(0, 3) << ':';
    if (pt2.size() <= 5)
        ostrm << pt2;
    else
        ostrm << pt2.substr(0, 2) << ".." << pt2.substr(pt2.size() - 2);

    return ostrm;
}

Edge::Edge(utp::SignedHeadNode const & i_shn)
    : m_shn(i_shn)
{
    HeadNode hn;
    if (!hn.ParseFromString(i_shn.headnode()))
        throwstream(InternalError, FILELINE << "failed to parse headNode");

    m_prev = make_pair(hn.fstag(), hn.prevref());
    m_root = make_pair(hn.fstag(), hn.rootref());
}

ostream &
operator<<(ostream & ostrm, Edge const & i_e)
{
    ostrm << i_e.m_prev << " -> " << i_e.m_root;
    return ostrm;
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
    if (ACE_OS::stat(sizepath.c_str(), &sb) != 0)
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: size file \""
                    << sizepath << "\" does not exist");
    
    if (! S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "FSBlockStore::destroy: size file \""
                    << sizepath << "\" not file");

    // Unlink the SIZE file
    unlink(sizepath.c_str());

    // Unlink the HEADS file
    string headspath = path + "/HEADS";
    unlink(headspath.c_str());

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
    if (mkdir(m_rootpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        throwstream(InternalError, FILELINE
                    << "mkdir " << m_rootpath << "failed: "
                    << ACE_OS::strerror(errno));

    // Make the BLOCKS subdir.
    m_blockspath = m_rootpath + "/BLOCKS";
    if (mkdir(m_blockspath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
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
          NotFoundError)
{
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

    // Read all of the existing SignedHeadNodes
    ifstream shnstrm(headspath().c_str());
    string linebuf;
    while (getline(shnstrm, linebuf))
    {
        SignedHeadNode shn;
        string data = Base64::decode(linebuf);
        int ok = shn.ParseFromString(data);
        if (!ok)
            throwstream(InternalError, FILELINE
                        << " SignedHeadNode deserialize failed");
        insert_head(shn);
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
FSBlockStore::bs_get_block_async(void const * i_keydata,
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

            int fd = open(blkpath.c_str(), O_RDONLY, S_IRUSR);
            bytes_read = read(fd, o_buffdata, i_buffsize);
            close(fd);

            if (bytes_read == -1) {
                throwstream(InternalError, FILELINE
                            << "FSBlockStore::bs_get_block: read failed: "
                            << strerror(errno));
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
FSBlockStore::bs_put_block_async(void const * i_keydata,
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
        LOG(lgr, 6, m_instname << ' ' << "bs_put_block");

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

            int fd = open(blkpath.c_str(),
                          O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
            if (fd == -1)
                throwstream(InternalError, FILELINE
                            << "FSBlockStore::bs_put_block: open failed on "
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
FSBlockStore::bs_refresh_start(uint64 i_rid)
    throw(InternalError,
          NotUniqueError)
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
}

void
FSBlockStore::bs_refresh_block_async(uint64 i_rid,
                                     void const * i_keydata,
                                     size_t i_keysize,
                                     BlockRefreshCompletion & i_cmpl,
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
        i_cmpl.br_missing(i_keydata, i_keysize, i_argp);
    else
        i_cmpl.br_complete(i_keydata, i_keysize, i_argp);
}
        
void
FSBlockStore::bs_refresh_finish(uint64 i_rid)
    throw(InternalError,
          NotFoundError)
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
}

void
FSBlockStore::bs_sync()
    throw(InternalError)
{
	//always synced to disk
}

void
FSBlockStore::bs_head_insert_async(SignedHeadNode const & i_shn,
                                   SignedHeadInsertCompletion & i_cmpl,
                                   void const * i_argp)
    throw(InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

    insert_head(i_shn);

    write_head(i_shn);

    i_cmpl.shi_complete(i_shn, i_argp);
}

void
FSBlockStore::bs_head_follow_async(SignedHeadNode const & i_shn,
                                   SignedHeadTraverseFunc & i_func,
                                   void const * i_argp)
    throw(InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

    // Obtain the seed from the seed edge.
    Edge edge(i_shn);
    NodeRef seed = edge.m_root;

    // Expand the seeds.
    NodeRefSet seeds;
    if (seed.second.size() == 0)
    {
        // There was no reference, we should use all roots w/ the same fstag.
        for (NodeRefSet::const_iterator it = m_roots.lower_bound(seed);
             it != m_roots.end() && it->first == seed.first;
             ++it)
        {
            NodeRef nr = *it;
            seeds.insert(nr);
        }
    }
    else
    {
        // Yes, see if we can find the seed in the rootmap
        EdgeMap::const_iterator pos = m_rootmap.find(seed);
        if (pos != m_rootmap.end())
        {
            // It's here, we can start with it.
            seeds.insert(pos->first);
        }
        else
        {
            // Can we find children of this node instead?
            EdgeMap::const_iterator it = m_prevmap.lower_bound(seed);
            EdgeMap::const_iterator end = m_prevmap.upper_bound(seed);
            for (; it != end; ++it)
            {
                seeds.insert(it->second->m_root);

                // We need to call the traverse function on the
                // children as well since they follow the seed.
                //
                i_func.sht_node(i_argp, it->second->m_shn);
            }
        }
    }

    // If our seeds collection is empty, we are out of luck.
    if (seeds.empty())
        i_func.sht_error(i_argp, NotFoundError("no starting seed found"));

    // Replace elements of the seed set w/ their children.
    bool done ;
    do
    {
        // Start optimistically
        done = true;

        // Find a seed with children.
        for (NodeRefSet::iterator it = seeds.begin(); it != seeds.end(); ++it)
        {
            NodeRef nr = *it;

            // Find all the children of this seed.
            NodeRefSet kids;
            EdgeMap::iterator kit = m_prevmap.lower_bound(nr);
            EdgeMap::iterator end = m_prevmap.upper_bound(nr);
            for (; kit != end; ++kit)
            {
                kids.insert(kit->second->m_root);

                // Call the traverse function on the kid.
                i_func.sht_node(i_argp, kit->second->m_shn);
            }

            // Were there kids?
            if (!kids.empty())
            {
                // Remove the parent from the set.
                seeds.erase(nr);

                // Insert the kids instead.
                seeds.insert(kids.begin(), kids.end());

                // Start over, we're not done.
                done = false;
                break;
            }
        }
    }
    while (!done);

    i_func.sht_complete(i_argp);
}

void
FSBlockStore::bs_head_furthest_async(SignedHeadNode const & i_shn,
                                     SignedHeadTraverseFunc & i_func,
                                     void const * i_argp)
    throw(InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_fsbsmutex);

    // Obtain the seed from the seed edge.
    Edge edge(i_shn);
    NodeRef seed = edge.m_root;

    // Expand the seeds.
    NodeRefSet seeds;
    if (seed.second.size() == 0)
    {
        // There was no reference, we should use all roots w/ the same fstag.
        for (NodeRefSet::const_iterator it = m_roots.lower_bound(seed);
             it != m_roots.end() && it->first == seed.first;
             ++it)
        {
            NodeRef nr = *it;
            seeds.insert(nr);
        }
    }
    else
    {
        // Yes, see if we can find the seed in the rootmap
        EdgeMap::const_iterator pos = m_rootmap.find(seed);
        if (pos != m_rootmap.end())
        {
            // It's here, we can start with it.
            seeds.insert(pos->first);
        }
        else
        {
            // Can we find children of this node instead?
            EdgeMap::const_iterator it = m_prevmap.lower_bound(seed);
            EdgeMap::const_iterator end = m_prevmap.upper_bound(seed);
            for (; it != end; ++it)
                seeds.insert(it->second->m_root);
        }
    }

    // If our seeds collection is empty, we are out of luck.
    if (seeds.empty())
        i_func.sht_error(i_argp, NotFoundError("no starting seed found"));

    // Replace elements of the seed set w/ their children.
    bool done ;
    do
    {
        // Start optimistically
        done = true;

        // Find a seed with children.
        for (NodeRefSet::iterator it = seeds.begin(); it != seeds.end(); ++it)
        {
            NodeRef nr = *it;

            // Find all the children of this seed.
            NodeRefSet kids;
            EdgeMap::iterator kit = m_prevmap.lower_bound(nr);
            EdgeMap::iterator end = m_prevmap.upper_bound(nr);
            for (; kit != end; ++kit)
            {
                LOG(lgr, 8, m_instname << ' ' << "adding kid " << *kit->second);
                kids.insert(kit->second->m_root);
            }

            // Were there kids?
            if (!kids.empty())
            {
                // Remove the parent from the set.
                LOG(lgr, 8, m_instname << ' ' << "removing parent " << nr);
                seeds.erase(nr);

                // Insert the kids instead.
                seeds.insert(kids.begin(), kids.end());

                // Start over, we're not done.
                done = false;
                break;
            }
        }
    }
    while (!done);

    // Call the traversal function on each remaining node.
    for (NodeRefSet::iterator it = seeds.begin(); it != seeds.end(); ++it)
    {
        EdgeMap::const_iterator pos = m_rootmap.find(*it);
        if (pos == m_rootmap.end())
            throwstream(InternalError, FILELINE << "missing rootmap entry");

        // We just use the first matching node ...
        
        i_func.sht_node(i_argp, pos->second->m_shn);
    }

    i_func.sht_complete(i_argp);
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
    ostrm << "RID:" << i_rid;
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
    if (unlink(blkpath.c_str()) != 0)
        throwstream(InternalError, FILELINE
                    << "unlink " << blkpath << " failed: "
                    << ACE_OS::strerror(errno));

    // Update the accounting.
    m_uncommitted -= eh->m_size;
}

void
FSBlockStore::insert_head(SignedHeadNode const & i_shn)
{
    // IMPORTANT - We presume the mutex is held by the caller.

    EdgeHandle eh = new Edge(i_shn);

    if (eh->m_prev == eh->m_root)
        throwstream(InternalError, FILELINE
                    << "we'd really rather not have self-loops");

    m_prevmap.insert(make_pair(eh->m_prev, eh));

    m_rootmap.insert(make_pair(eh->m_root, eh));

    // Remove any nodes which we preceede from the root set.
    m_roots.erase(eh->m_root);

    // Are we in the root set so far?  If our previous node
    // hasn't been seen we insert ourselves ...
    //
    if (m_rootmap.find(eh->m_prev) == m_rootmap.end())
        m_roots.insert(eh->m_prev);
}

void
FSBlockStore::write_head(SignedHeadNode const & i_shn)
{
    string linebuf;
    i_shn.SerializeToString(&linebuf);
    m_headsstrm << Base64::encode(linebuf.data(), linebuf.size()) << endl;
}

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
