#ifndef FSBSFactory_h__
#define FSBSFactory_h__

/// @file FSBSFactory.h
/// FSBS Factory

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "BlockStoreFactory.h"

#include "fsbsexp.h"

class FSBS_EXP FSBSFactory
    : public ACE_Service_Object
    , public utp::BlockStoreFactory
{
public:
    /// Default constructor.
    FSBSFactory();

    /// Destructor
    virtual ~FSBSFactory();

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
              utp::NotUniqueError,
              utp::ValueError);

    virtual utp::BlockStoreHandle bsf_open(std::string const & i_instname,
                                           utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError,
              utp::ValueError);

    virtual void bsf_destroy(utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError,
              utp::ValueError);

protected:
};

ACE_STATIC_SVC_DECLARE(FSBSFactory)
ACE_SVC_FACTORY_DECLARE(FSBSFactory)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // FSBSFactory_h__
