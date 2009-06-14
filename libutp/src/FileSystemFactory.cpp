#include <map>

#include "Log.h"
#include "Except.h"
#include "FileSystem.h"
#include "FileSystemFactory.h"

#include "utplog.h"

using namespace std;

namespace {

typedef map<string, utp::FileSystemFactory *> FileSystemFactoryMap;

FileSystemFactoryMap g_fsfm;

} // end namespace



namespace utp {

void
FileSystemFactory::register_factory(string const & i_name,
                                    FileSystemFactory * i_fsfp)
{
    LOG(lgr, 4, "register_factory " << i_name);

    g_fsfm[i_name] = i_fsfp;
}

FileSystemHandle
FileSystemFactory::mkfs(string const & i_name,
                        BlockStoreHandle const & i_bsh,
                        string const & i_fsid,
                        string const & i_pass,
                        StringSeq const & i_args)
{
    LOG(lgr, 4, "mkfs " << i_name);

    FileSystemFactoryMap::iterator pos = g_fsfm.find(i_name);
    if (pos == g_fsfm.end())
        throwstream(ValueError,
                    "filesystem factory for \"" << i_name << "\" not found");

    return pos->second->fsf_mkfs(i_bsh, i_fsid, i_pass, i_args);
}
                          
FileSystemHandle
FileSystemFactory::mount(string const & i_name,
                         BlockStoreHandle const & i_bsh,
                         string const & i_fsid,
                         string const & i_pass,
                         StringSeq const & i_args)
{
    LOG(lgr, 4, "mount " << i_name);

    FileSystemFactoryMap::iterator pos = g_fsfm.find(i_name);
    if (pos == g_fsfm.end())
        throwstream(ValueError,
                    "filesystem factory for \"" << i_name << "\" not found");

    return pos->second->fsf_mount(i_bsh, i_fsid, i_pass, i_args);
}
                          
FileSystemFactory::~FileSystemFactory()
{
}

} // end namespace utp
