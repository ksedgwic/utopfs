#include "BlockStore.h"
#include "Except.h"
#include "Log.h"
#include "StreamCipher.h"

#include "utfslog.h"

#include "DirNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

DirNode::DirNode()
{
    LOG(lgr, 4, "CTOR");
}

DirNode::~DirNode()
{
    LOG(lgr, 4, "DTOR");
}

void
DirNode::traverse(string const & i_entry,
                  string const & i_rmndr,
                  TraverseFunc & i_trunc)
{
    throwstream(InternalError, FILELINE
                << "DirNode::traverse unimplemented");
}

int
DirNode::getattr(struct stat * o_statbuf)
{
    throwstream(InternalError, FILELINE
                << "DirNode::getattr unimplemented");
}

int
DirNode::open(std::string const & i_entry, int i_flags)
{
    throwstream(InternalError, FILELINE
                << "DirNode::open unimplemented");
}

int
DirNode::readdir(off_t i_offset, FileSystem::DirEntryFunc & o_entryfunc)
{
    throwstream(InternalError, FILELINE
                << "DirNode::readdir unimplemented");
}

pair<string, string>
DirNode::pathsplit(string const & i_path)
{
    // The path needs to start with a '/'.
    if (i_path[0] != '/')
        throwstream(InternalError, FILELINE
                    << "path \"" << i_path << "\" doesn't begin with '/'");

    // Return the path component up to the next '/'.
    string::size_type pos = i_path.find('/', 1);

    if (pos == string::npos)
        return make_pair(i_path.substr(1, pos), "");
    else
        return make_pair(i_path.substr(1, pos - 1), i_path.substr(pos));
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
