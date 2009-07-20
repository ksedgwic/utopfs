#ifndef S3BSFactory_h__
#define S3BSFactory_h__

/// @file S3BSFactory.h
/// S3BS Factory

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "BlockStoreFactory.h"

#include "s3bsexp.h"

class S3BS_EXP S3BSFactory
    : public ACE_Service_Object
    , public utp::BlockStoreFactory
{
public:
    /// Default constructor.
    S3BSFactory();

    /// Destructor
    virtual ~S3BSFactory();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

    /// BlockStoreFactory methods.

    virtual utp::BlockStoreHandle bsf_create(size_t i_size,
                                             utp::StringSeq const & i_args);

    virtual utp::BlockStoreHandle bsf_open(utp::StringSeq const & i_args);

protected:
};

ACE_STATIC_SVC_DECLARE(S3BSFactory)
ACE_SVC_FACTORY_DECLARE(S3BSFactory)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3BSFactory_h__
