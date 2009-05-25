#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "BlockStore.h"
#include "Except.h"
#include "Log.h"
#include "StreamCipher.h"

#include "utfslog.h"

#include "DirNode.h"

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

    // Return the path component up to the next '/'.
    string::size_type pos = i_path.find('/', 1);

    if (pos == string::npos)
        return make_pair(i_path.substr(1, pos), "");
    else
        return make_pair(i_path.substr(1, pos - 1), i_path.substr(pos));
}

DirNode::DirNode()
{
    LOG(lgr, 4, "CTOR");
}

DirNode::DirNode(Context & i_ctxt, Digest const & i_dig)
    : FileNode(i_ctxt, i_dig)
{
    LOG(lgr, 4, "CTOR " << i_dig);

    // NOTE - We use protobuf components individually here
    // because ParseFromArray insists that the entire input
    // stream be consumed.
    //
    ArrayInputStream input(data(), size());
    CodedInputStream decoder(&input);
    if (!m_dir.ParseFromCodedStream(&decoder))
        throwstream(InternalError, FILELINE
                    << "dir deserialization failed");
}

DirNode::~DirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
DirNode::traverse(Context & i_ctxt,
                  unsigned int i_flags,
                  string const & i_entry,
                  string const & i_rmndr,
                  TraverseFunc & i_trav)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

    if (i_flags & TF_PARENT)
    {
        // This is a parent traversal.

        // Are we at the parent?
        if (i_rmndr.empty())
        {
            // Yes, this is the parent.
            i_trav.tf_parent(i_ctxt, *this, i_entry);

            if (i_flags & TF_UPDATE)
            {
                // Need to redo the lookup because it might have changed.
                fnh = lookup(i_ctxt, i_entry);
                if (fnh)
                    i_trav.tf_update(i_ctxt, *this, i_entry, fnh->digest());
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
            dnh->traverse(i_ctxt, i_flags, ps.first, ps.second, i_trav);

            if (i_flags & TF_UPDATE)
                i_trav.tf_update(i_ctxt, *this, i_entry, fnh->digest());
        }
    }
    else
    {
        // This is a leaf traversal.
        if (!fnh)
            throw ENOENT;

        if (i_rmndr.empty())
        {
            i_trav.tf_leaf(i_ctxt, *fnh);

            if (i_flags & TF_UPDATE)
                i_trav.tf_update(i_ctxt, *this, i_entry, fnh->digest());
        }
        else
        {
            DirNodeHandle dnh = dynamic_cast<DirNode *>(&*fnh);

            // If the cast didn't work then this wasn't a directory.
            if (!dnh)
                throw ENOTDIR;

            pair<string, string> ps = pathsplit(i_rmndr);
            dnh->traverse(i_ctxt, i_flags, ps.first, ps.second, i_trav);

            if (i_flags & TF_UPDATE)
                i_trav.tf_update(i_ctxt, *this, i_entry, fnh->digest());
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
            LOG(lgr, 6, Digest(ent.digest()) << " " << ent.name());
        }
    }

    // Persist our directory map.
    bool rv = m_dir.SerializeToArray(data(), size());
    if (!rv)
        throwstream(InternalError, FILELINE << "dir serialization error");

    // Let the FileNode do all the hard work ...
    FileNode::persist(i_ctxt);
}

void
DirNode::update(Context & i_ctxt,
                string const & i_entry,
                Digest const & i_dig)
{
    LOG(lgr, 6, "update " << i_entry << " " << i_dig);

    // Does this entry exist already?
    for (int i = 0; i < m_dir.entry_size(); ++i)
    {
        Directory::Entry * entp = m_dir.mutable_entry(i);
        if (entp->name() == i_entry)
        {
            entp->set_digest(i_dig);
            persist(i_ctxt);
            return;
        }
    }

    // If we get here it needs to be added ...
    Directory::Entry * entp = m_dir.add_entry();
    entp->set_name(i_entry);
    entp->set_digest(i_dig);
    persist(i_ctxt);
}

int
DirNode::getattr(Context & i_ctxt, struct stat * o_statbuf)
{
    throwstream(InternalError, FILELINE
                << "DirNode::getattr unimplemented");
}

int
DirNode::open(Context & i_ctxt,
              string const & i_entry,
              int i_flags)
{
    FileNodeHandle fnh = lookup(i_ctxt, i_entry);

    if (i_flags & O_CREAT)
    {
        // We're creating this file.
        if (!fnh)
        {
            // Create a new file.
            fnh = new FileNode();

            // Persist it (sets the digest).
            fnh->persist(i_ctxt);

            // Insert into our Directory collection.
            Directory::Entry * de = m_dir.add_entry();
            de->set_name(i_entry);
            de->set_digest(fnh->digest());

            // Insert into the cache.
            m_cache.insert(make_pair(i_entry, fnh));
        }
    }
    else
    {
        // The file needs to exist.
        if (!fnh)
            throw ENOENT;
    }

    return 0;
}

int
DirNode::readdir(Context & i_ctxt,
                 off_t i_offset,
                 FileSystem::DirEntryFunc & o_entryfunc)
{
    throwstream(InternalError, FILELINE
                << "DirNode::readdir unimplemented");
}

FileNodeHandle
DirNode::lookup(Context & i_ctxt, string const & i_entry)
{
    // Do we have a cached entry for this name?
    FileNodeHandle fnh;
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
                FileNodeHandle fnh =
                    new FileNode(i_ctxt, m_dir.entry(i).digest());
                m_cache.insert(make_pair(i_entry, fnh));
                return fnh;
            }
        }
    }

    return NULL;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
