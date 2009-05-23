#include "Log.h"

#include "utfslog.h"

#include "SpecialDirNode.h"
#include "RootDirNode.h"

using namespace std;
using namespace utp;

#define SPECIALDIR	".utopfs"

namespace UTFS {

RootDirNode::RootDirNode()
{
    LOG(lgr, 4, "CTOR");

    m_sdh = new SpecialDirNode;
}

RootDirNode::~RootDirNode()
{
    LOG(lgr, 4, "DTOR");
}

FileNodeHandle
RootDirNode::resolve(std::string const & i_path)
{
    // We are the root node.
    if (i_path == "/")
        return this;

    pair<string, string> comp = pathsplit(i_path);

    // Check for the special .utopfs directory.
    if (comp.first == SPECIALDIR)
        return comp.second.empty() ?
            FileNodeHandle(&*m_sdh) :
            m_sdh->resolve(comp.second);

    // Otherwise delegate to the base class.
    return DirNode::resolve(i_path);
}

pair<DirNodeHandle, string>
RootDirNode::resolve_parent(std::string const & i_path)
{
    pair<string, string> comp = pathsplit(i_path);

    // If the remainder is empty *we* are the parent.
    if (comp.second.empty())
        return make_pair(DirNodeHandle(this), comp.first);

    // Check for the special .utopfs directory.
    if (comp.first == SPECIALDIR)
        return m_sdh->resolve_parent(comp.second);

    // Otherwise delegate to the base class.
    return DirNode::resolve_parent(i_path);
}

int
RootDirNode::getattr(struct stat * o_stbuf)
{
    o_stbuf->st_mode = S_IFDIR | 0755;
    o_stbuf->st_nlink = 2;
    return 0;
}

int
RootDirNode::readdir(off_t i_offset, FileSystem::DirEntryFunc & o_entryfunc)
{
    // FIXME - BOGUS!  We need to have actual dynamic entries!
    o_entryfunc.def_entry(".", NULL, 0);
    o_entryfunc.def_entry("..", NULL, 0);
    o_entryfunc.def_entry(SPECIALDIR, NULL, 0);
    return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
