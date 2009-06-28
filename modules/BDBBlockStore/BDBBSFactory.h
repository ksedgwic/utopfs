#ifndef BDBBSFactory_h__
#define BDBBSFactory_h__

/// @file BDBBSFactory.h
/// BDBBS Factory

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "BlockStoreFactory.h"

#include "bdbbsexp.h"

class BDBBS_EXP BDBBSFactory
    : public ACE_Service_Object
    , public utp::BlockStoreFactory
{
public:
    /// Default constructor.
    BDBBSFactory();

    /// Destructor
    virtual ~BDBBSFactory();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

    /// BlockStoreFactory methods.

    virtual utp::BlockStoreHandle bsf_create(utp::StringSeq const & i_args);

    virtual utp::BlockStoreHandle bsf_open(utp::StringSeq const & i_args);

protected:
};

ACE_STATIC_SVC_DECLARE(BDBBSFactory)
ACE_SVC_FACTORY_DECLARE(BDBBSFactory)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // BDBBSFactory_h__
