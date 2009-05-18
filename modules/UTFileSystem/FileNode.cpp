#include "Log.h"
#include "Except.h"

#include "utfslog.h"

#include "FileNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

FileNode::FileNode()
{
    LOG(lgr, 4, "CTOR");
}

FileNode::~FileNode()
{
    LOG(lgr, 4, "DTOR");
}

int
FileNode::getattr(struct stat * o_statbuf)
{
    throwstream(InternalError, FILELINE
                << "FileNode::getattr unimplemented");
}

int
FileNode::read(void * o_bufptr, size_t i_size, off_t i_off)
{
    throwstream(InternalError, FILELINE
                << "FileNode::read unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
