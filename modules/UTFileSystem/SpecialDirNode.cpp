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
    : m_version(new SpecialFileNode(utopfs_ver_str))
{
    LOG(lgr, 4, "CTOR");
}

SpecialDirNode::~SpecialDirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
SpecialDirNode::traverse(Context & i_ctxt,
                         unsigned int i_flags,
                         string const & i_entry,
                         string const & i_rmndr,
                         TraverseFunc & i_trav)
{
    // Check for special files.
    if (i_entry == "version" && i_rmndr.empty())
    {
        i_trav.tf_parent(i_ctxt, *this, i_entry);
        i_trav.tf_leaf(i_ctxt, *m_version);

        // SPECIAL CASE: We don't update digests ...
        // i_trav.tf_update(*this, i_entry, m_version->digest());
    }

    else
    {
        // We only have special files.
        throw ENOENT;
    }
}

int
SpecialDirNode::getattr(Context & i_ctxt, struct stat * o_stbuf)
{
    o_stbuf->st_mode = S_IFDIR | 0755;
    o_stbuf->st_nlink = 2;
    return 0;
}

int
SpecialDirNode::open(Context & i_ctxt,
                     std::string const & i_entry,
                     int i_flags)
{
    if (i_entry != "version")
        return ENOENT;

    if ((i_flags & 3) != O_RDONLY)
        return EACCES;

    return 0;
}

int
SpecialDirNode::readdir(Context & i_ctxt,
                        off_t i_offset,
                        FileSystem::DirEntryFunc & o_entryfunc)
{
    o_entryfunc.def_entry(".", NULL, 0);
    o_entryfunc.def_entry("..", NULL, 0);
    o_entryfunc.def_entry("version", NULL, 0);
    return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
