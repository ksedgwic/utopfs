#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "BlockStore.h"
#include "Except.h"
#include "Log.h"
#include "BlockCipher.h"

#include "utfslog.h"

#include "DirNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace UTFS {

void
DirNode::NodeTraverseFunc::nt_update(Context & i_ctxt,
                                     DirNode & i_dn,
                                     std::string const & i_entry,
                                     BlockRef const & i_ref)
{
    i_dn.update(i_ctxt, i_entry, i_ref);
}

pair<string, string>
DirNode::pathsplit(string const & i_path)
{
    // The path needs to start with a '/'.
    if (i_path[0] != '/')
        throwstream(InternalError, FILELINE
                    << "path \"" << i_path << "\" doesn't begin with '/'");

    // Return the path component up to the next '/'.
    string::size_type pos = i_path.find('/', 1);

    if (pos == string::npos)
        return make_pair(i_path.substr(1, pos), "");
    else
        return make_pair(i_path.substr(1, pos - 1), i_path.substr(pos));
}

DirNode::DirNode(mode_t i_mode)
    : FileNode(i_mode)
{
    LOG(lgr, 4, "CTOR");

    // We are a directory, not a file.
    mode_t m = mode();	// Fetch current bits
    m &= ~S_IFMT;		// Turn off all IFMT bits.
    m |= S_IFDIR;		// Turn on the directory bits.
    mode(m);			// Set the bits.

    // Directories have a link count of 2 when they are created ("."
    // and "..").
    nlink(2);
}

DirNode::DirNode(FileNode const & i_fn)
    : FileNode(i_fn)
{
    LOG(lgr, 4, "CTOR");

    deserialize();
}

DirNode::DirNode(Context & i_ctxt, BlockRef const & i_ref)
    : FileNode(i_ctxt, i_ref)
{
    LOG(lgr, 4, "CTOR " << i_ref);

    deserialize();
}

DirNode::~DirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
DirNode::node_traverse(Context & i_ctxt,
                       unsigned int i_flags,
                       string const & i_entry,
                       string const & i_rmndr,
                       NodeTraverseFunc & i_trav)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

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
                fnh = lookup(i_ctxt, i_entry);
                if (fnh)
                {
                    // Insert or update.
                    i_trav.nt_update(i_ctxt, *this, i_entry, fnh->bn_blkref());
                }
                else
                {
                    // Erase.
                    i_trav.nt_update(i_ctxt, *this, i_entry, BlockRef());
                }
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
                i_trav.nt_update(i_ctxt, *this, i_entry, fnh->bn_blkref());
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
                i_trav.nt_update(i_ctxt, *this, i_entry, fnh->bn_blkref());
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
                i_trav.nt_update(i_ctxt, *this, i_entry, fnh->bn_blkref());
        }
    }

}

void
DirNode::persist(Context & i_ctxt)
{
    LOG(lgr, 6, "persist");
    if (lgr.is_enabled(6))
    {
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            Directory::Entry const & ent = m_dir.entry(i);
            LOG(lgr, 6, "[" << i << "]: "
                << BlockRef(ent.blkref()) << " " << ent.name());
        }
    }

    // Persist our directory map.
    ACE_OS::memset(bn_data(), '\0', bn_size());
    bool rv = m_dir.SerializeToArray(bn_data(), bn_size());
    if (!rv)
        throwstream(InternalError, FILELINE << "dir serialization error");

    // Let the FileNode do all the hard work ...
    FileNode::bn_persist(i_ctxt);
}

void
DirNode::update(Context & i_ctxt,
                string const & i_entry,
                BlockRef const & i_ref)
{
    LOG(lgr, 6, "update " << i_entry << " " << i_ref);

    // Was this a delete?
    if (!i_ref)
    {
        persist(i_ctxt);
        return;
    }

    // Does this entry exist already?
    for (int i = 0; i < m_dir.entry_size(); ++i)
    {
        Directory::Entry * entp = m_dir.mutable_entry(i);
        if (entp->name() == i_entry)
        {
            entp->set_blkref(i_ref);
            persist(i_ctxt);
            return;
        }
    }

    // If we get here it needs to be added ...
    Directory::Entry * entp = m_dir.add_entry();
    entp->set_name(i_entry);
    entp->set_blkref(i_ref);
    persist(i_ctxt);
}

int
DirNode::mknod(Context & i_ctxt,
               string const & i_entry,
               mode_t i_mode,
               dev_t i_dev)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

    // We're creating this file.
    if (!fnh)
    {
        // Create a new file.
        fnh = new FileNode(i_mode);

        // Persist it (sets the blkref).
        fnh->bn_persist(i_ctxt);

        // Insert into our Directory collection.
        Directory::Entry * de = m_dir.add_entry();
        de->set_name(i_entry);
        de->set_blkref(fnh->bn_blkref());

        // Insert into the cache.
        m_cache.insert(make_pair(i_entry, fnh));
    }

    return 0;
}

int
DirNode::mkdir(Context & i_ctxt,
               string const & i_entry,
               mode_t i_mode)
{
    // The entry better not already exist.
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);
    if (fnh)
        throw EEXIST;

    // Create the new directory node.
    DirNodeHandle dnh = new DirNode(i_mode);

    // Persist it (sets the digest).
    dnh->persist(i_ctxt);

    // Insert into our Directory collection.
    Directory::Entry * de = m_dir.add_entry();
    de->set_name(i_entry);
    de->set_blkref(dnh->bn_blkref());

    // Insert into the cache.
    m_cache.insert(make_pair(i_entry, dnh));

    // Increment our link count.
    nlink(nlink() + 1);

    return 0;
}

int
DirNode::unlink(Context & i_ctxt, string const & i_entry)
{
    // Lookup the entry.
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

    // It needs to exist.
    if (!fnh)
        throw ENOENT;

    // We don't remove directories.
    if (S_ISDIR(fnh->mode()))
        throw EISDIR;

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
    for (int i = 0; i < m_dir.entry_size() - 1; ++i)
    {
        if (m_dir.entry(i).name() == i_entry)
        {
            // Get mutable reference to this entry.
            Directory::Entry * me = m_dir.mutable_entry(i);

            // Copy last entry into this slot.
            *me = m_dir.entry(m_dir.entry_size() - 1);

            // Deed is done ...
            break;
        }
    }
    m_dir.mutable_entry()->RemoveLast();

    // Remove from the cache.
    m_cache.erase(i_entry);

    return 0;
}

int
DirNode::open(Context & i_ctxt,
              string const & i_entry,
              int i_flags)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

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

FileNodeHandle
DirNode::lookup(Context & i_ctxt, string const & i_entry)
{
    // Do we have a cached entry for this name?
    EntryMap::const_iterator pos = m_cache.find(i_entry);
    if (pos != m_cache.end())
    {
        return pos->second;
    }
    else
    {
        // Is it in the directories digest table?
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            if (m_dir.entry(i).name() == i_entry)
            {
                FileNodeHandle nh =
                    new FileNode(i_ctxt, m_dir.entry(i).blkref());

                // Is it really a directory?
                if (S_ISDIR(nh->mode()))
                    nh = new DirNode(*nh);

                m_cache.insert(make_pair(i_entry, nh));
                return nh;
            }
        }
    }

    return NULL;
}

void
DirNode::deserialize()
{
    // NOTE - We use protobuf components individually here
    // because ParseFromArray insists that the entire input
    // stream be consumed.
    //
    ArrayInputStream input(bn_data(), bn_size());
    CodedInputStream decoder(&input);
    if (!m_dir.ParseFromCodedStream(&decoder))
        throwstream(InternalError, FILELINE
                    << "dir deserialization failed");

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

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
