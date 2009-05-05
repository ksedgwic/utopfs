#ifndef FSBSSvc_h__
#define FSBSSvc_h__

/// @file FSBSSvc.h
/// FSBS Service

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "utpfwd.h"

#include "fsbsexp.h"

class FSBS_EXP FSBSSvc
    : public ACE_Service_Object
{
public:
    /// Default constructor.
    FSBSSvc();

    /// Destructor
    virtual ~FSBSSvc();

    /// ACE_Service_Object methods.

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

protected:
};

ACE_STATIC_SVC_DECLARE(FSBSSvc)
ACE_SVC_FACTORY_DECLARE(FSBSSvc)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // FSBSSvc_h__
