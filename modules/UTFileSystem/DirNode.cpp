#include "BlockStore.h"
#include "Except.h"
#include "Log.h"
#include "StreamCipher.h"

#include "utfslog.h"

#include "DirNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

DirNode::DirNode()
{
    LOG(lgr, 4, "CTOR");
}

DirNode::~DirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
DirNode::traverse(BlockStoreHandle const & i_bsh,
                  StreamCipher & i_cipher,
                  string const & i_entry,
                  string const & i_rmndr,
                  TraverseFunc & i_trav)
{
    // Do we have a cached entry for this name?
    FileNodeHandle fnh;
    EntryMap::const_iterator pos = m_cache.find(i_entry);
    if (pos != m_cache.end())
    {
        fnh = pos->second;
    }
    else
    {
        // Is it in the directories digest table?
        for (int i = 0; i < m_dir.entry_size(); ++i)
        {
            if (m_dir.entry(i).name() == i_entry)
            {
                fnh = new FileNode(i_bsh, i_cipher, m_dir.entry(i).digest());
                m_cache.insert(make_pair(i_entry, fnh));
            }
        }
    }

    if (!fnh)
        throw ENOENT;

    if (i_rmndr.empty())
    {
        // We're at the leaf.
        i_trav.tf_parent(*this, i_entry);
        i_trav.tf_leaf(*fnh);
    }
    else
    {
        DirNodeHandle dnh = dynamic_cast<DirNode *>(&*fnh);

        // If the cast didn't work then this wasn't a directory.
        if (!dnh)
            throw ENOTDIR;

        // We have more traversing to do.
        pair<string, string> ps = pathsplit(i_rmndr);
        dnh->traverse(i_bsh, i_cipher, ps.first, ps.second, i_trav);
    }

    i_trav.tf_update(*this, i_entry, fnh->digest());
}

int
DirNode::getattr(struct stat * o_statbuf)
{
    throwstream(InternalError, FILELINE
                << "DirNode::getattr unimplemented");
}

int
DirNode::open(std::string const & i_entry, int i_flags)
{
    throwstream(InternalError, FILELINE
                << "DirNode::open unimplemented");
}

int
DirNode::readdir(off_t i_offset, FileSystem::DirEntryFunc & o_entryfunc)
{
    throwstream(InternalError, FILELINE
                << "DirNode::readdir unimplemented");
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

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
