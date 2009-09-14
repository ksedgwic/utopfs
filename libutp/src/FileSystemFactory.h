#ifndef utp_FileSystemFactory_h__
#define utp_FileSystemFactory_h__

/// @file FileSystemFactory.h
/// Abstract FileSystemFactory Interface.

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "BlockStore.h"
#include "Except.h"
#include "FileSystem.h"
#include "RC.h"
#include "Types.h"

namespace utp {

class UTP_EXP FileSystemFactory
{
public:
    // Called by module init to register factory object.
    static void register_factory(std::string const & i_name,
                                 FileSystemFactory * i_fsfp);

    // Called at runtime to create new blockstore.
    static FileSystemHandle mkfs(std::string const & i_name,
                                 BlockStoreHandle const & i_bsh,
                                 std::string const & i_fsid,
                                 std::string const & i_pass,
                                 std::string const & i_uname,
                                 std::string const & i_gname,
                                 StringSeq const & i_args);

    // Called at runtime to open existing blockstore.
    static FileSystemHandle mount(std::string const & i_name,
                                  BlockStoreHandle const & i_bsh,
                                  std::string const & i_fsid,
                                  std::string const & i_pass,
                                  StringSeq const & i_args);

    // Helper function to map uid to string (mkfs wants string).
    static std::string mapuid(uid_t i_uid);

    // Helper function to map gid to string (mkfs wants string).
    static std::string mapgid(gid_t i_gid);

    virtual ~FileSystemFactory();

    virtual FileSystemHandle fsf_mkfs(BlockStoreHandle const & i_bsh,
                                      std::string const & i_fsid,
                                      std::string const & i_pass,
                                      std::string const & i_uname,
                                      std::string const & i_gname,
                                      StringSeq const & i_args) = 0;

    virtual FileSystemHandle fsf_mount(BlockStoreHandle const & i_bsh,
                                       std::string const & i_fsid,
                                       std::string const & i_pass,
                                       StringSeq const & i_args) = 0;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_FileSystemFactory_h__
