#include "Log.h"

#include "utfslog.h"

#include "SpecialSockNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

SpecialSockNode::SpecialSockNode()
    : FileNode(0444)
{
    LOG(lgr, 4, "CTOR");

    // We are a socket, not a file.
    mode_t m = mode();	// Fetch current bits
    m &= ~S_IFMT;		// Turn off all IFMT bits.
    m |= S_IFSOCK;		// Turn on the directory bits.
    mode(m);			// Set the bits.
}

SpecialSockNode::~SpecialSockNode()
{
    LOG(lgr, 4, "DTOR");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
