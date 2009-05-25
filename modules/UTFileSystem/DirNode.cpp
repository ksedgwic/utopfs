#include "BlockStore.h"
#include "Except.h"
#include "Log.h"
#include "StreamCipher.h"

#include "utfslog.h"

#include "DirNode.h"

using namespace std;
using namespace utp;

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
    if (i_flags & O_CREAT)
    {
        // We're creating this file.
    }
    else
    {
        // The file needs to exist.
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
