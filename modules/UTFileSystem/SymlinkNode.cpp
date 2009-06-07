#include "Log.h"

#include "utfslog.h"

#include "SymlinkNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

SymlinkNode::SymlinkNode(string const & i_path)
    : FileNode(0777)
{
    LOG(lgr, 4, "CTOR");

    // We are a symlink, not a file.
    mode_t m = mode();	// Fetch current bits
    m &= ~S_IFMT;		// Turn off all IFMT bits.
    m |= S_IFLNK;		// Turn on the symlink bits.
    mode(m);			// Set the bits.

    // Store the path in the inline block.
    ACE_OS::memcpy(bn_data(), i_path.data(), i_path.size());

    // Set the size of the data.
    size(i_path.size());
}

SymlinkNode::SymlinkNode(FileNode const & i_fn)
    : FileNode(i_fn)
{
    LOG(lgr, 4, "CTOR " << bn_blkref());
}

SymlinkNode::~SymlinkNode()
{
    LOG(lgr, 4, "DTOR " << bn_blkref());
}

int
SymlinkNode::readlink(Context & i_ctxt,
                      char * o_obuf,
                      size_t i_size)
{
    size_t sz = min(size(), i_size);

    ACE_OS::memcpy(o_obuf, bn_data(), sz);

    return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
