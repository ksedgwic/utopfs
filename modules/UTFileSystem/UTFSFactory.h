#ifndef UTFS_UTFSFactory_h__
#define UTFS_UTFSFactory_h__

/// @file UTFSFactory.h
/// UTFS Service

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "FileSystemFactory.h"

#include "utfsexp.h"

class UTFS_EXP UTFSFactory
    : public ACE_Service_Object
    , public utp::FileSystemFactory
{
public:
    /// Default constructor.
    UTFSFactory();

    /// Destructor
    virtual ~UTFSFactory();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

    /// FileSystemFactory methods.

    virtual utp::FileSystemHandle fsf_mkfs(utp::BlockStoreHandle const & i_bsh,
                                           std::string const & i_fsid,
                                           std::string const & i_pass,
                                           std::string const & i_uname,
                                           std::string const & i_gname,
                                           utp::StringSeq const & i_args);

    virtual utp::FileSystemHandle fsf_mount(utp::BlockStoreHandle const & i_bsh,
                                            std::string const & i_fsid,
                                            std::string const & i_pass,
                                            utp::StringSeq const & i_args);

protected:
};

ACE_STATIC_SVC_DECLARE(UTFSFactory)
ACE_SVC_FACTORY_DECLARE(UTFSFactory)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_UTFSFactory_h__
