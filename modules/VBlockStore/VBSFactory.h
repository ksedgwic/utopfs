#ifndef VBSFactory_h__
#define VBSFactory_h__

/// @file VBSFactory.h
/// VBS Factory

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "BlockStoreFactory.h"

#include "vbsexp.h"

class VBS_EXP VBSFactory
    : public ACE_Service_Object
    , public utp::BlockStoreFactory
{
public:
    /// Default constructor.
    VBSFactory();

    /// Destructor
    virtual ~VBSFactory();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

    /// BlockStoreFactory methods.

    virtual utp::BlockStoreHandle bsf_create(std::string const & i_instname,
                                             size_t i_size,
                                             utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotUniqueError);

    virtual utp::BlockStoreHandle bsf_open(std::string const & i_instname,
                                           utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError);

    virtual void bsf_destroy(utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError);

protected:
};

ACE_STATIC_SVC_DECLARE(VBSFactory)
ACE_SVC_FACTORY_DECLARE(VBSFactory)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSFactory_h__
