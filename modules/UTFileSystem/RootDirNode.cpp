#include "Log.h"

#include "utfslog.h"

#include "SpecialDirNode.h"
#include "RootDirNode.h"
#include "BlockRef.h"

using namespace std;
using namespace utp;

#define SPECIALDIR	".utopfs"

namespace UTFS {

RootDirNode::RootDirNode()
    : DirNode(0777)
{
    LOG(lgr, 4, "CTOR");

    m_sdh = new SpecialDirNode;
}

RootDirNode::RootDirNode(Context & i_ctxt, BlockRef const & i_ref)
    : DirNode(i_ctxt, i_ref)
{
    LOG(lgr, 4, "CTOR");

    m_sdh = new SpecialDirNode;
}

RootDirNode::~RootDirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
RootDirNode::node_traverse(Context & i_ctxt,
                           unsigned int i_flags,
                           string const & i_entry,
                           string const & i_rmndr,
                           NodeTraverseFunc & i_trav)
{
    // Check for the root directory itself.
    if (i_entry.empty())
    {
        // If a parent traversal was requested then we have issues.
        if (i_flags & NT_PARENT)
            throw ENOENT;

        i_trav.nt_leaf(i_ctxt, *this);

        // I think our caller has to do any required update.
    }

    // Check for the special .utopfs directory.
    else if (i_entry == SPECIALDIR)
    {
        if (i_rmndr.empty())
        {
            i_trav.nt_leaf(i_ctxt, *m_sdh);
        }
        else
        {
            pair<string, string> ps = pathsplit(i_rmndr);
            m_sdh->node_traverse(i_ctxt, i_flags, ps.first, ps.second, i_trav);
        }

        // SPECIAL CASE: Changes to the .utopfs dir don't update
        // our digest.
        //
        // i_trav.nt_update(*this, i_entry, m_sdh->digest());
    }

    // Otherwise defer to the base class.
    else
    {
        DirNode::node_traverse(i_ctxt, i_flags, i_entry, i_rmndr, i_trav);
    }
}

int
RootDirNode::getattr(Context & i_ctxt, struct stat * o_stbuf)
{
    // Let the base class do most of the work.
    DirNode::getattr(i_ctxt, o_stbuf);

    // We have have one pseudo-directory, bump the link count.
    o_stbuf->st_nlink += 1;

    return 0;
}

int
RootDirNode::readdir(Context & i_ctxt,
                     off_t i_offset,
                     FileSystem::DirEntryFunc & o_entryfunc)
{
    // First call the base class to emit the real entries.
    DirNode::readdir(i_ctxt, i_offset, o_entryfunc);

    // Then add our pseudo directory.
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
