#include "Log.h"
#include "Except.h"

#include "Random.h"

#include "utfslog.h"

#include "FileNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

FileNode::FileNode()
{
    LOG(lgr, 4, "CTOR");

    Random::fill(m_initvec, sizeof(m_initvec));
}

FileNode::~FileNode()
{
    LOG(lgr, 4, "DTOR");
}

void
FileNode::persist()
{
    throwstream(InternalError, FILELINE
                << "FileNode::persist unimplemented");
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

int
FileNode::write(void const * i_data, size_t i_size, off_t i_off)
{
    throwstream(InternalError, FILELINE
                << "FileNode::write unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
