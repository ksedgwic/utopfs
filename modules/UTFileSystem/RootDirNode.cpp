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

void
RootDirNode::traverse(string const & i_entry,
                      string const & i_rmndr,
                      TraverseFunc & i_trav)
{
    // Check for the special .utopfs directory.
    if (i_entry == SPECIALDIR)
    {
        if (i_rmndr.empty())
        {
            i_trav.tf_leaf(*m_sdh);
        }
        else
        {
            pair<string, string> ps = pathsplit(i_rmndr);
            m_sdh->traverse(ps.first, ps.second, i_trav);
        }

        // SPECIAL CASE: Changes to the .utopfs dir don't update
        // our digest.
        //
        // i_trav.tf_update(*this, i_entry, m_sdh->digest());
    }

    // Otherwise defer to the base class.
    else
    {
        DirNode::traverse(i_entry, i_rmndr, i_trav);
    }
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
