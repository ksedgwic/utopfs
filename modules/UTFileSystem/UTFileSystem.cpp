#include <sstream>
#include <string>

#include "Log.h"

#include "UTFileSystem.h"
#include "utfslog.h"

#include "Base32.h"

using namespace std;
using namespace utp;

namespace UTFS {

UTFileSystem::UTFileSystem()
{
    LOG(lgr, 4, "CTOR");
}

UTFileSystem::~UTFileSystem()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
UTFileSystem::fs_mkfs(std::string const & i_path)
    throw (utp::InternalError,
           utp::ValueError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_mkfs unimplemented");
}

void
UTFileSystem::fs_mount(std::string const & i_path)
    throw (utp::InternalError,
           utp::ValueError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_mount unimplemented");
}

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_statbuf)
    throw (utp::InternalError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_getattr unimplemented");
}

int
UTFileSystem::fs_open(string const & i_path,
                      int i_flags)
    throw (utp::InternalError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_open unimplemented");
}

int
UTFileSystem::fs_read(string const & i_path,
                      void * o_bufptr,
                      size_t i_size,
                      off_t i_off)
    throw (utp::InternalError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_read unimplemented");
}

int
UTFileSystem::fs_readdir(string const & i_path,
                         off_t i_offset,
                         DirEntryFunc & o_entryfunc)
    throw (utp::InternalError)
{
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_readdir unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
