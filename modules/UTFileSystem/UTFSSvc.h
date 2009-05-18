#ifndef UTFS_UTFSSvc_h__
#define UTFS_UTFSSvc_h__

/// @file UTFSSvc.h
/// UTFS Service

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "utfsexp.h"

class UTFS_EXP UTFSSvc
    : public ACE_Service_Object
{
public:
    /// Default constructor.
    UTFSSvc();

    /// Destructor
    virtual ~UTFSSvc();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

protected:
};

ACE_STATIC_SVC_DECLARE(UTFSSvc)
ACE_SVC_FACTORY_DECLARE(UTFSSvc)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_UTFSSvc_h__
