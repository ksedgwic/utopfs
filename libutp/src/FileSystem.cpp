#include "Except.h"
#include "FileSystem.h"

namespace {

utp::FileSystemHandle g_bsh;    

} // end namespace

namespace utp {

void
FileSystem::instance(FileSystemHandle const & i_bsh)
{
    g_bsh = i_bsh;
}

FileSystemHandle
FileSystem::instance()
    throw (NotFoundError)
{
    if (!g_bsh)
        throwstream(NotFoundError, "no FileSystem instance");

    return g_bsh;
}

FileSystem::~FileSystem()
{
}

} // end namespace utp
