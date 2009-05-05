#ifndef DFLGSvc_h__
#define DFLGSvc_h__

/// @file DFLGSvc.h
/// Defines the Default Logging Service Object.

#include <ace/svc_export.h>
#include <ace/Service_Object.h>
#include <ace/Service_Config.h>

#include "dflgexp.h"

// class DFLG::Console;
// class DFLG::File;

/// The Default Logging Service Object.
///
/// This ACE_Service_Object is loaded by the ACE_Service_Configurator.
/// It is responsible for managing a singleton instance of
/// DefaultLogging.

class DFLG_EXP DFLGSvc : public ACE_Service_Object
{
public:
    /// Default constructor.
    DFLGSvc();

    /// Destructor
    virtual ~DFLGSvc();

    virtual int init(int argc, char *argv[]);

    virtual int fini(void);

    virtual int suspend(void);

    virtual int resume(void);

protected:
    DFLG::Console *			m_conslogger;
    DFLG::File *			m_filelogger;
};

ACE_STATIC_SVC_DECLARE(DFLGSvc)
ACE_SVC_FACTORY_DECLARE(DFLGSvc)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLGSvc_h__
