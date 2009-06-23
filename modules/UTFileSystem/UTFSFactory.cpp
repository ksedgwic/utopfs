#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"
#include "FileSystemFactory.h"

#include "utfslog.h"

#include "UTFileSystem.h"

#include "UTFSFactory.h"

using namespace std;
using namespace utp;

UTFSFactory::UTFSFactory()
{
}

UTFSFactory::~UTFSFactory()
{
}

int
UTFSFactory::init(int argc, char * argv[])
{
    FileSystemFactory::register_factory("UTFS", this);

    return 0;
}

int
UTFSFactory::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
UTFSFactory::suspend()
{
    return 0;
}

int
UTFSFactory::resume()
{
    return 0;
}

FileSystemHandle
UTFSFactory::fsf_mkfs(BlockStoreHandle const & i_bsh,
                      string const & i_fsid,
                      string const & i_pass,
                      string const & i_uname,
                      string const & i_gname,
                      StringSeq const & i_args)
{
    FileSystemHandle fsh = new UTFS::UTFileSystem();
    fsh->fs_mkfs(i_bsh, i_fsid, i_pass, i_uname, i_gname, i_args);
    return fsh;
}

FileSystemHandle
UTFSFactory::fsf_mount(BlockStoreHandle const & i_bsh,
                       string const & i_fsid,
                       string const & i_pass,
                       StringSeq const & i_args)
{
    FileSystemHandle fsh = new UTFS::UTFileSystem();
    fsh->fs_mount(i_bsh, i_fsid, i_pass, i_args);
    return fsh;
}

ACE_SVC_FACTORY_DEFINE(UTFSFactory)

ACE_STATIC_SVC_DEFINE(UTFSFactory,
                      ACE_TEXT ("UTFSFactory"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (UTFSFactory),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
