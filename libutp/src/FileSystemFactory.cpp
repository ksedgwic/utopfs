#include <map>

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

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
                        string const & i_uname,
                        string const & i_gname,
                        StringSeq const & i_args)
{
    LOG(lgr, 4, "mkfs " << i_name);

    FileSystemFactoryMap::iterator pos = g_fsfm.find(i_name);
    if (pos == g_fsfm.end())
        throwstream(ValueError,
                    "filesystem factory for \"" << i_name << "\" not found");

    return pos->second->fsf_mkfs(i_bsh, i_fsid, i_pass,
                                 i_uname, i_gname, i_args);
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

string
FileSystemFactory::mapuid(uid_t uid)
{
    struct passwd pw;
    struct passwd * pwp;
    char buf[1024];
    int rv = getpwuid_r(uid, &pw, buf, sizeof(buf), &pwp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getpwuid_r failed: " << strerror(rv));
    if (!pwp)
        throwstream(InternalError, FILELINE << "no password entry found");

    return pwp->pw_name;
}

string
FileSystemFactory::mapgid(gid_t gid)
{
    struct group gr;
    struct group * grp;
    char buf[1024];
    int rv = getgrgid_r(gid, &gr, buf, sizeof(buf), &grp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getgrgid_r failed: " << strerror(rv));
    if (!grp)
        throwstream(InternalError, FILELINE << "no group entry found");

    return grp->gr_name;
}

FileSystemFactory::~FileSystemFactory()
{
}

} // end namespace utp
