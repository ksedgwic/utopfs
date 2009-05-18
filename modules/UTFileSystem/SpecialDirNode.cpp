#include "Log.h"

#include "utfslog.h"

#include "SpecialDirNode.h"
#include "SpecialFileNode.h"

using namespace std;
using namespace utp;

namespace {

    char const * utopfs_ver_str = "utopfs version 0.1\n";

} // end namespace

namespace UTFS {

SpecialDirNode::SpecialDirNode()
{
    LOG(lgr, 4, "CTOR");
}

SpecialDirNode::~SpecialDirNode()
{
    // LOG(lgr, 4, "DTOR");
}

FileNodeHandle
SpecialDirNode::resolve(std::string const & i_path)
{
    pair<string, string> comp = pathcomp(i_path);

    // Check for the special files.
    if (comp.first == "version" && comp.second.empty())
        return new SpecialFileNode(utopfs_ver_str);

    // We only have special files ...
    else
        throw ENOENT;
}

pair<DirNodeHandle, string>
SpecialDirNode::resolve_parent(std::string const & i_path)
{
    if (i_path == "/version")
        return make_pair(this, "version");

    else
        throw ENOENT;
}

int
SpecialDirNode::getattr(struct stat * o_stbuf)
{
    o_stbuf->st_mode = S_IFDIR | 0755;
    o_stbuf->st_nlink = 2;
    return 0;
}

int
SpecialDirNode::open(std::string const & i_entry, int i_flags)
{
    if (i_entry != "version")
        return ENOENT;

    if ((i_flags & 3) != O_RDONLY)
        return EACCES;

    return 0;
}

int
SpecialDirNode::readdir(off_t i_offset, FileSystem::DirEntryFunc & o_entryfunc)
{
    o_entryfunc(".", NULL, 0);
    o_entryfunc("..", NULL, 0);
    o_entryfunc("version", NULL, 0);
    return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
