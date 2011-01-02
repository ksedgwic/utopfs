#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "T64.h"
#include "Log.h"

#include "File.h"
#include "Console.h"
#include "DFLGSvc.h"

using namespace std;
using namespace utp;
using namespace DFLG;

DFLGSvc::DFLGSvc()
    : m_conslogger(NULL)
    , m_filelogger(NULL)
{
}

DFLGSvc::~DFLGSvc()
{
    if (m_conslogger)
        delete m_conslogger;

    if (m_filelogger)
        delete m_filelogger;
}

int
DFLGSvc::init(int argc, char * argv[])
{
    // defaults
    int loglevel = 9;
    bool consenable = false;
    int conslevel = 0;
    bool fileenable = false;
    int filelevel = 9;
    string filepath = "default.log";
    bool fileflush = true;

    // usage:  -l <level>               global logging mask [9]
    //         -c <console-level>       enables console, level [console-off]
    //         -f <file-level>          enables file, level [9]
    //         -p <file-path>           enables file, path [default.log]
    //         -x                       don't flush file lines [do-flush]

    ACE_Get_Opt getopt(argc, argv, "c:f:l:p:x", 0);
    for (int c; (c = getopt()) != -1; ) {
        switch (c) {
        case 'c':
            consenable = true;
            conslevel = atoi(getopt.opt_arg());
            break;
        case 'f':
            fileenable = true;
            filelevel = atoi(getopt.opt_arg());
            break;
        case 'l':
            loglevel = atoi(getopt.opt_arg());
            break;
        case 'p':
            fileenable = true;
            filepath = getopt.opt_arg();
            break;
        case 'x':
            fileflush = false;
            break;
        }
    }

    // Have any of the logging parameters been overridden by
    // environment variables?
    //
    char * filelevelenv = ACE_OS::getenv("UTOPFS_LOG_FILELEVEL");
    if (filelevelenv && *filelevelenv)
        filelevel = atoi(filelevelenv);
    char * filepathenv = ACE_OS::getenv("UTOPFS_LOG_FILEPATH");
    if (filepathenv && *filepathenv)
        filepath = filepathenv;
    char * conslevelenv = ACE_OS::getenv("UTOPFS_LOG_CONSLEVEL");
    if (conslevelenv && *conslevelenv)
    {
        consenable = true;
        conslevel = atoi(conslevelenv);
    }
    
#if 0
    PMapHandle ns = Config::getInstance()->getNS("");

    conslevel = ns->getInt32OrDefault("log.consolelevel", conslevel);
    if (conslevel > 0)
        consenable = true;

    filelevel = ns->getInt32OrDefault("log.filelevel", filelevel);
    if (filelevel > 0)
        fileenable = true;

    filepath = ns->getStringOrDefault("log.filepath", filepath);
    if (!filepath.empty())
        fileenable = true;
#endif

    try {
        if (consenable) {
            m_conslogger = new Console(conslevel);
            theRootLogCategory.logger_add(m_conslogger, true);
        }

        if (fileenable) {
            m_filelogger = new File(filelevel, filepath, fileflush);
            theRootLogCategory.logger_add(m_filelogger, true);
        }

        theRootLogCategory.level(loglevel, true);

        LOG(theRootLogCategory, 1, " ------------------------------");
        LOG(theRootLogCategory, 1, "LOG STARTED AT " << T64::now());

        return 0;
    }
    catch(const std::exception & ex) {
        LOG(theRootLogCategory, 1, "EXCEPTION: " << ex.what());
        cerr << ex.what() << endl;
    }
    catch(...) {
        LOG(theRootLogCategory, 1, "UNKNOWN EXCEPTION");
        cerr << "UNKNOWN EXCEPTION" << endl;
    }
    return -1;
}

int
DFLGSvc::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
DFLGSvc::suspend()
{
    LOG(theRootLogCategory, 1, "LOG SUSPEND AT " << T64::now());
    LOG(theRootLogCategory, 1, " ------------------------------");


    if (m_filelogger)
        theRootLogCategory.logger_rem(m_filelogger);

    if (m_conslogger)
        theRootLogCategory.logger_rem(m_conslogger);

    return 0;
}

int
DFLGSvc::resume()
{
    if (m_conslogger)
        theRootLogCategory.logger_add(m_conslogger);

    if (m_filelogger)
        theRootLogCategory.logger_add(m_filelogger);

    LOG(theRootLogCategory, 1, " ------------------------------");
    LOG(theRootLogCategory, 1, "LOG RESUMED AT " << T64::now());

    return 0;
}

ACE_SVC_FACTORY_DEFINE(DFLGSvc)

ACE_STATIC_SVC_DEFINE(DFLGSvc,
                      ACE_TEXT ("DFLGSvc"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (DFLGSvc),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
