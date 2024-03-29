#include <sstream>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "BlockCipher.h"
#include "BlockStore.h"
#include "Except.h"
#include "Log.h"

#include "utfslog.h"

#include "BlockNodeCache.h"
#include "Context.h"
#include "DirNode.h"
#include "SymlinkNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace UTFS {

pair<string, string>
DirNode::pathsplit(string const & i_path)
{
    // The path needs to start with a '/'.
    if (i_path[0] != '/')
        throwstream(InternalError, FILELINE
                    << "path \"" << i_path << "\" doesn't begin with '/'");

    // Find the path component up to the next '/'.
    string::size_type pos = i_path.find('/', 1);

    // Are we at the end of the path?
    if (pos == string::npos)
    {
        // Yes, the "remainder" is empty.
        return make_pair(i_path.substr(1, pos), "");
    }
    else
    {
        // Nope, there is more.

        // Find the start of the next path component.
        //
        // We need to treat multiple consecutive slashes like
        // a single slash.
        //
        string::size_type pos2 = i_path.find_first_not_of('/', pos);

        // Are we at the end of the path now?
        if (pos2 == string::npos)
        {
            // Yes, the "remainder" is empty.
            return make_pair(i_path.substr(1, pos), "");
        }
        else
        {
            // Return the adjusted remainder.
            return make_pair(i_path.substr(1, pos - 1),
                             i_path.substr(pos2 - 1));
        }
    }
}

DirNode::DirNode(mode_t i_mode,
                 string const & i_uname,
                 string const & i_gname)
    : FileNode(i_mode, i_uname, i_gname)
{
    LOG(lgr, 6, "CTOR");

    // We are a directory, not a file.
    mode_t m = mode();	// Fetch current bits
    m &= ~S_IFMT;		// Turn off all IFMT bits.
    m |= S_IFDIR;		// Turn on the directory bits.
    mode(m);			// Set the bits.

    // Directories have a link count of 2 when they are created ("."
    // and "..").
    nlink(2);
}

DirNode::DirNode(Context & i_ctxt, FileNode const & i_fn)
    : FileNode(i_fn)
{
    LOG(lgr, 6, "CTOR " << bn_blkref());

    deserialize(i_ctxt);
}

DirNode::DirNode(Context & i_ctxt, BlockRef const & i_ref)
    : FileNode(i_ctxt, i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);

    deserialize(i_ctxt);
}

DirNode::~DirNode()
{
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

BlockRef const &
DirNode::bn_flush(Context & i_ctxt)
{
    // If we aren't dirty then we just return our current reference.
    if (!bn_isdirty())
        return bn_blkref();

    // Traverse the items in our dirty cache, flush them, insert them
    // in the block node cache and remove from the dirty cache.
    //
    while (!m_dirty.empty())
    {
        // Grab the first node.
        EntryMap::iterator pos = m_dirty.begin();

        // Flush it out to the block store.
        BlockRef const & blkref = pos->second->bn_flush(i_ctxt);

        // Update the reference in our directory.
        update(pos->first, blkref);

        // Insert in the clean node cache.
        i_ctxt.m_bncachep->insert(pos->second);

        // Erase from the dirty collection.
        m_dirty.erase(pos);
    }

    if (lgr.is_enabled(6))
    {
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            Directory::Entry const & ent = m_dir.entry(i);
            LOG(lgr, 6, "[" << i << "]: "
                << BlockRef(ent.blkref()) << " " << ent.name());
        }
    }

    // Note the old size first.
    size_t oldsz = size();

    // Serialize the data to a stream.
    ostringstream ostrm;
    bool ok = m_dir.SerializeToOstream(&ostrm);
    if (!ok)
        throwstream(InternalError, FILELINE << "dir serialization error");

    size_t newsz = ostrm.str().size();

    // Write the stream to the underlying file.
    int rv = FileNode::write(i_ctxt, ostrm.str().data(), newsz, 0);
    if (rv < 0)
        throwstream(InternalError, FILELINE
                    << "Trouble writing directory data: "
                    << ACE_OS::strerror(-rv));

    // Truncate the file size to the directory size.
    if (newsz < oldsz)
        FileNode::truncate(i_ctxt, ostrm.str().size());

    // Let the FileNode do all the rest of the work ...
    return FileNode::bn_flush(i_ctxt);
}

void
DirNode::bn_tostream(std::ostream & ostrm) const
{
    FileNode::bn_tostream(ostrm);
    ostrm << ' ' << "DirNode";
}

size_t
DirNode::rb_refresh(Context & i_ctxt, uint64 i_rid)
{
    size_t nblocks = 0;

    for (int i = 0; i < m_dir.entry_size(); ++i)
    {
        Directory::Entry const & ent = m_dir.entry(i);

        // Do we have a cached dirty object?
        EntryMap::iterator pos = m_dirty.find(ent.name());
        if (pos != m_dirty.end())
        {
            nblocks += pos->second->rb_refresh(i_ctxt, i_rid);
        }
        else
        {
            // Does the clean cache have one?
            FileNodeHandle nh;
            BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(ent.blkref());
            if (bnh)
            {
                // Upcast to FileNode, it is at least that.
                nh = dynamic_cast<FileNode *>(&*bnh);
            }
            else
            {
                nh = new FileNode(i_ctxt, ent.blkref());

                // Is it really a directory?  Upgrade object ...
                if (S_ISDIR(nh->mode()))
                    nh = new DirNode(i_ctxt, *nh);

                // Is it really a symlink?  Upgrade object ...
                else if (S_ISLNK(nh->mode()))
                    nh = new SymlinkNode(*nh);

                // Should we insert it in the clean cache here?
                //
                // Seems like the refresh kinda wrecks the cache.
                // Maybe let them evaporate when we are done
                // traversing instead.
            }

            nblocks += nh->rb_refresh(i_ctxt, i_rid);
        }
    }

    return nblocks + FileNode::rb_refresh(i_ctxt, i_rid);
}
                            
void
DirNode::node_traverse(Context & i_ctxt,
                       unsigned int i_flags,
                       string const & i_entry,
                       string const & i_rmndr,
                       NodeTraverseFunc & i_trav)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, i_flags & NT_READONLY);

    if (i_flags & NT_PARENT)
    {
        // This is a parent traversal.

        // Are we at the parent?
        if (i_rmndr.empty())
        {
            // Yes, this is the parent.
            i_trav.nt_parent(i_ctxt, *this, i_entry);

            if (i_flags & NT_UPDATE)
            {
                // Need to redo the lookup because it might have changed.
                fnh = lookup(i_ctxt, i_entry, i_flags & NT_READONLY);
                update(i_ctxt, i_entry, fnh);
            }
        }
        else
        {
            // Nope, there's more to go.
            if (!fnh)
                throw ENOENT;

            DirNodeHandle dnh = dynamic_cast<DirNode *>(&*fnh);

            // If the cast didn't work then this wasn't a directory.
            if (!dnh)
                throw ENOTDIR;

            pair<string, string> ps = pathsplit(i_rmndr);
            dnh->node_traverse(i_ctxt, i_flags, ps.first, ps.second, i_trav);

            if (i_flags & NT_UPDATE)
                update(i_ctxt, i_entry, dnh);
        }
    }
    else
    {
        // This is a leaf traversal.
        if (!fnh)
            throw ENOENT;

        if (i_rmndr.empty())
        {
            i_trav.nt_leaf(i_ctxt, *fnh);

            if (i_flags & NT_UPDATE)
                update(i_ctxt, i_entry, fnh);
        }
        else
        {
            DirNodeHandle dnh = dynamic_cast<DirNode *>(&*fnh);

            // If the cast didn't work then this wasn't a directory.
            if (!dnh)
                throw ENOTDIR;

            pair<string, string> ps = pathsplit(i_rmndr);
            dnh->node_traverse(i_ctxt, i_flags, ps.first, ps.second, i_trav);

            if (i_flags & NT_UPDATE)
                update(i_ctxt, i_entry, dnh);
        }
    }

}

size_t
DirNode::numentries() const
{
    return m_dir.entry_size();
}

int
DirNode::mknod(Context & i_ctxt,
               string const & i_entry,
               mode_t i_mode,
               dev_t i_dev,
               string const & i_uname,
               string const & i_gname)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);
    if (fnh)
        throw EEXIST;

    // Create a new file.
    fnh = new FileNode(i_mode, i_uname, i_gname);

    // Insert into the dirty cache.
    m_dirty.insert(make_pair(i_entry, fnh));

    // Insert a placeholder in the directory.
    update(i_entry, BlockRef());

    bn_isdirty(true);

    return 0;
}

int
DirNode::mkdir(Context & i_ctxt,
               string const & i_entry,
               mode_t i_mode,
               string const & i_uname,
               string const & i_gname)
{
    // The entry better not already exist.
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);
    if (fnh)
        throw EEXIST;

    // Create the new directory node.
    DirNodeHandle dnh = new DirNode(i_mode, i_uname, i_gname);

    // Insert into the dirty cache.
    m_dirty.insert(make_pair(i_entry, dnh));

    // Insert a placeholder in the directory.
    update(i_entry, BlockRef());

    // Increment our link count.
    nlink(nlink() + 1);

    bn_isdirty(true);

    return 0;
}

int
DirNode::unlink(Context & i_ctxt, string const & i_entry, bool i_dirstoo)
{
    // Lookup the entry.
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);

    // It needs to exist.
    if (!fnh)
        throw ENOENT;

    // We don't remove directories.
    if (!i_dirstoo && S_ISDIR(fnh->mode()))
        throw EISDIR;

    remove(i_ctxt, i_entry);

    return 0;
}

int
DirNode::rmdir(Context & i_ctxt, string const & i_entry)
{
    // Lookup the entry.
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);

    // It needs to exist.
    if (!fnh)
        throw ENOENT;

    // We only remove directories.
    if (!S_ISDIR(fnh->mode()))
        throw ENOTDIR;

    // Directory needs to be empty.
    DirNodeHandle dnh = dynamic_cast<DirNode *>(&*fnh);
    if (dnh->numentries() != 0)
        throw ENOTEMPTY;

    remove(i_ctxt, i_entry);

    return 0;
}

int
DirNode::symlink(Context & i_ctxt,
                 string const & i_entry,
                 string const & i_opath)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);

    // It needs to not exist.
    if (fnh)
        throw EEXIST;

    // Create the symbolic link.
    fnh = new SymlinkNode(i_opath);

    // Insert into the dirty cache.
    m_dirty.insert(make_pair(i_entry, fnh));

    // Insert a placeholder in the directory.
    update(i_entry, BlockRef());

    bn_isdirty(true);

    return 0;
    
}

BlockRef
DirNode::linksrc(Context & i_ctxt,
                 string const & i_entry)
{
    return find_blkref(i_ctxt, i_entry);
}

int
DirNode::linkdst(Context & i_ctxt,
                 string const & i_entry,
                 BlockRef const & i_blkref,
                 bool i_force)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);

    // Does the target path exist already?
    if (fnh)
    {
        if (i_force)
            remove(i_ctxt, i_entry);
        else
            throw EEXIST;
    }

    // Insert into our Directory collection.
    Directory::Entry * de = m_dir.add_entry();
    de->set_name(i_entry);
    de->set_blkref(i_blkref);

    bn_isdirty(true);

    return 0;
}

int
DirNode::open(Context & i_ctxt, string const & i_entry, int i_flags)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry, false);

    // The file needs to exist.
    if (!fnh)
        throw ENOENT;

    return 0;
}

int
DirNode::readdir(Context & i_ctxt,
                 off_t i_offset,
                 FileSystem::DirEntryFunc & o_entryfunc)
{
    // Add all the entries in our digest table.
    for (int i = 0; i < m_dir.entry_size(); ++i)
        o_entryfunc.def_entry(m_dir.entry(i).name(), NULL, 0);

    // Add some old favorites.
    o_entryfunc.def_entry(".", NULL, 0);
    o_entryfunc.def_entry("..", NULL, 0);

    return 0;
}

void
DirNode::update(Context & i_ctxt,
                 string const & i_entry,
                 FileNodeHandle const & i_fnh)
{
    LOG(lgr, 6, "update " << i_entry);

    mtime(T64::now());

    // If it's not a removal, insert in the dirty cache.
    if (i_fnh)
        m_dirty.insert(make_pair(i_entry, i_fnh));

    bn_isdirty(true);
}

FileNodeHandle
DirNode::lookup(Context & i_ctxt,
                string const & i_entry,
                bool i_readonly)
{
    // Do we have a cached dirty object for this name?
    EntryMap::const_iterator pos = m_dirty.find(i_entry);
    if (pos != m_dirty.end())
    {
        return pos->second;
    }
    else
    {
        // Is it in the directories digest table?
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            Directory::Entry const & ent = m_dir.entry(i);

            if (ent.name() == i_entry)
            {
                // Is it in the clean cache?
                FileNodeHandle nh;
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(ent.blkref());
                if (bnh)
                {
                    // Upcast to FileNode, it is at least that.
                    nh = dynamic_cast<FileNode *>(&*bnh);
                }
                else
                {
                    // If we are read-only bail and come back with a
                    // write lock ...
                    //
                    if (i_readonly)
                        throw OperationError(i_entry);

                    nh = new FileNode(i_ctxt, ent.blkref());

                    // Is it really a directory?  Upgrade object ...
                    if (S_ISDIR(nh->mode()))
                        nh = new DirNode(i_ctxt, *nh);

                    // Is it really a symlink?  Upgrade object ...
                    else if (S_ISLNK(nh->mode()))
                        nh = new SymlinkNode(*nh);

                    // Insert this in the clean cache.
                    i_ctxt.m_bncachep->insert(nh);
                }

                return nh;
            }
        }
    }

    return NULL;
}

BlockRef
DirNode::find_blkref(Context & i_ctxt, string const & i_entry)
{
    // If it is in the cache we need to flush it so we
    // have a valid block reference.
    EntryMap::iterator pos = m_dirty.find(i_entry);
    if (pos != m_dirty.end())
    {
        FileNodeHandle fnh = pos->second;

        // Flush the node to get a valid reference.
        BlockRef ref = fnh->bn_flush(i_ctxt);

        // Insert it into the clean cache.
        i_ctxt.m_bncachep->insert(fnh);

        // Remove it from the dirty cache.
        m_dirty.erase(pos);

        // Update the ref in the entries table.
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            if (m_dir.entry(i).name() == i_entry)
            {
                m_dir.mutable_entry(i)->set_blkref(ref);
                break;
            }
        }
        
        return ref;
    }

    // Is it in the directories digest table?
    for (int i = 0; i < m_dir.entry_size(); ++i)
        if (m_dir.entry(i).name() == i_entry)
            return BlockRef(m_dir.entry(i).blkref());

    // Not found, nil reference.
    return BlockRef();
}

void
DirNode::deserialize(Context & i_ctxt)
{
    // Allocate a buffer for the directory entries.
    string buf(size(), '\0');

    // Copy the data into the buffer.
    int rv = FileNode::read(i_ctxt, &buf[0], buf.size(), 0);
    if (rv < 0)
        throwstream(InternalError, FILELINE
                    << "Trouble reading directory data: "
                    << ACE_OS::strerror(-rv));

    // Parse the directory object.
    int ok = m_dir.ParseFromString(buf);
    if (!ok)
        throwstream(InternalError, FILELINE
                    << "dir deserialization failed");

    // Log our entries if logging is verbose.
    LOG(lgr, 6, "deserialize");
    if (lgr.is_enabled(6))
    {
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            Directory::Entry const & ent = m_dir.entry(i);
            LOG(lgr, 6, "[" << i << "]: "
                << BlockRef(ent.blkref()) << " " << ent.name());
        }
    }
}

void
DirNode::update(string const & i_entry, BlockRef const & i_blkref)
{
    LOG(lgr, 6, "update " << i_entry << ' ' << i_blkref);

    bool found = false;

    for (int i = 0; i < m_dir.entry_size(); ++i)
    {
        Directory::Entry * entp = m_dir.mutable_entry(i);
        if (entp->name() == i_entry)
        {
            entp->set_blkref(i_blkref);
            found = true;
            break;
        }
    }

    if (!found)
    {
        Directory::Entry * entp = m_dir.add_entry();
        entp->set_name(i_entry);
        entp->set_blkref(i_blkref);
    }
}

void
DirNode::remove(Context & i_ctxt, string const & i_entry)
{
    LOG(lgr, 6, "remove " << i_entry);

    // Remove from our Directory collection.
    //
    // Wow, this loop sucks.  I think it is what is required though.
    // See /usr/include/protobuf/repeated_field.h: "RemoveLast".
    //
    // NOTE - We swap the item w/ the last item and shorten the list.
    //
    // No need to scan to the last item since we know the item is
    // in the list.
    //
    BlockRef ref;
    for (int i = 0; i < m_dir.entry_size() - 1; ++i)
    {
        if (m_dir.entry(i).name() == i_entry)
        {
            ref = m_dir.entry(i).blkref();

            // Get mutable reference to this entry.
            Directory::Entry * me = m_dir.mutable_entry(i);

            // Copy last entry into this slot.
            *me = m_dir.entry(m_dir.entry_size() - 1);

            // Deed is done ...
            break;
        }
    }
    m_dir.mutable_entry()->RemoveLast();

    // Remove from the dirty cache.
    m_dirty.erase(i_entry);

    // Remove any clean cache entry.
    if (ref)
        i_ctxt.m_bncachep->remove(ref);

    // Now we're dirty.
    bn_isdirty(true);
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
