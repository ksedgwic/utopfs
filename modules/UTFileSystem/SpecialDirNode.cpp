#include "Log.h"

#include "utfslog.h"

#include "SpecialDirNode.h"
#include "SpecialFileNode.h"
#include "SymlinkNode.h"

using namespace std;
using namespace utp;

namespace {

    char const * utopfs_ver_str = "utopfs version 0.3 [devel]\n";

} // end namespace

namespace UTFS {

SpecialDirNode::SpecialDirNode()
    : DirNode(0555, "root", "root")
    , m_version(new SpecialFileNode(utopfs_ver_str))
{
    LOG(lgr, 4, "CTOR");
}

SpecialDirNode::~SpecialDirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
SpecialDirNode::node_traverse(Context & i_ctxt,
                              unsigned int i_flags,
                              string const & i_entry,
                              string const & i_rmndr,
                              NodeTraverseFunc & i_trav)
{
    // Is this the version file?
    if (i_entry == "version" && i_rmndr.empty())
    {
        i_trav.nt_parent(i_ctxt, *this, i_entry);
        i_trav.nt_leaf(i_ctxt, *m_version);
    }

    // Is this the control socket?
    else if (i_entry == "control" && i_rmndr.empty())
    {
        i_trav.nt_parent(i_ctxt, *this, i_entry);

        if (!m_control)
            throw ENOENT;

        i_trav.nt_leaf(i_ctxt, *m_control);
    }

    else
    {
        // We only have special files.
        throw ENOENT;
    }
}

int
SpecialDirNode::getattr(Context & i_ctxt, struct statstb * o_stbuf)
{
    o_stbuf->st_mode = S_IFDIR | 0755;
    o_stbuf->st_nlink = 2;
    return 0;
}

int
SpecialDirNode::symlink(Context & i_ctxt,
                        string const & i_entry,
                        string const & i_opath)
{
    if (i_entry != "control")
        return ENOENT;
        
    m_control = new SymlinkNode(i_opath);

    return 0;
}

int
SpecialDirNode::open(Context & i_ctxt,
                     std::string const & i_entry,
                     int i_flags)
{
    if (i_entry == "version")
        return ((i_flags & 3) == O_RDONLY) ? 0 : EACCES;

    else if (i_entry == "control")
        return 0;

    return ENOENT;
}

int
SpecialDirNode::readdir(Context & i_ctxt,
                        off_t i_offset,
                        FileSystem::DirEntryFunc & o_entryfunc)
{
    o_entryfunc.def_entry(".", NULL, 0);
    o_entryfunc.def_entry("..", NULL, 0);
    o_entryfunc.def_entry("version", NULL, 0);
    if (m_control)
        o_entryfunc.def_entry("control", NULL, 0);
    return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
