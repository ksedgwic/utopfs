#include <typeinfo>
#include <stdexcept>

#include <ace/Reactor.h>
#include <ace/Service_Config.h>
#include <ace/Task.h>
#include <ace/TP_Reactor.h>

#include "pyutpinit.h"
#include "statemap.h"

#include "pyblockstore.h"
#include "pybsstat.h"
#include "pydirentryfunc.h"
#include "pyfilesystem.h"
#include "pyshe.h"
#include "pystat.h"
#include "pystatvfs.h"
#include "pyutplog.h"

using namespace std;

namespace utp {

// Instantiate error objects.
PyObject * ErrorObject;
PyObject * InternalErrorObject;
PyObject * OperationErrorObject;
PyObject * NotFoundErrorObject;
PyObject * NotUniqueErrorObject;
PyObject * ValueErrorObject;
PyObject * ParseErrorObject;
PyObject * VerificationErrorObject;
PyObject * NoSpaceErrorObject;

void
PyError_FromException(std::exception const & ex)
{
    PyObject * dict = PyDict_New();
    PyDict_SetItemString(dict, "what", PyString_FromString(ex.what()));

    Exception const * exp = dynamic_cast<Exception const *>(&ex);
    if (exp)
    {
        PyDict_SetItemString(dict, "details",
                             PyString_FromString(exp->details()));

        switch (exp->type())
        {
        case Exception::T_BASE:
            return PyErr_SetObject(ErrorObject, dict);
        case Exception::T_INTERNAL:
            return PyErr_SetObject(InternalErrorObject, dict);
        case Exception::T_OPERATION:
            return PyErr_SetObject(OperationErrorObject, dict);
        case Exception::T_NOTFOUND:
            return PyErr_SetObject(NotFoundErrorObject, dict);
        case Exception::T_NOTUNIQUE:
            return PyErr_SetObject(NotUniqueErrorObject, dict);
        case Exception::T_VALUE:
            return PyErr_SetObject(ValueErrorObject, dict);
        case Exception::T_PARSE:
            return PyErr_SetObject(ParseErrorObject, dict);
        case Exception::T_VERIFICATION:
            return PyErr_SetObject(VerificationErrorObject, dict);
        case Exception::T_NOSPACE:
            return PyErr_SetObject(NoSpaceErrorObject, dict);
        default:
            return PyErr_SetObject(ErrorObject, dict);
        }
    }
        
    // We weren't able to find one of our specific exceptions, map
    // the std::exception ..
    //
    PyErr_SetObject(ErrorObject, dict);
    return;
}

static PyMethodDef module_methods[] =
{
    /* No methods */
    {NULL,		NULL}		/* sentinel */
};

class ThreadPool : public ACE_Task_Base
{
public:
    ThreadPool(ACE_Reactor * i_reactor, int i_numthreads)
        : m_reactor(i_reactor)
    {
        LOG(lgr, 4, "ThreadPool CTOR");

        if (activate(THR_NEW_LWP | THR_JOINABLE, i_numthreads) != 0)
            abort();
    }
    
    ~ThreadPool()
    {
        LOG(lgr, 4, "ThreadPool DTOR");
        wait();
    }

    virtual int svc(void)
    {
        LOG(lgr, 4, "ThreadPool starting");

        // Run the Reactor event loop.  Catch exceptions and report but keep
        // the threads running ...
        //
        while (true)
        {
            try
            {
                m_reactor->run_reactor_event_loop();
            }

            catch (exception const & ex)
            {
                cerr << "caught std::exception: " << ex.what() << endl;
            }
            catch (...)
            {
                cerr << "caught UNKNOWN EXCEPTION" << endl;
            }
        }

        LOG(lgr, 4, "ThreadPool finished");
        return 0;
    }

private:
    ACE_Reactor *		m_reactor;
};

} // end namespace utp

extern "C" {

using namespace utp;

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_utp(void)
{
    // Initialize ACE
    int argc = 0;
    char * argv[1] = { NULL };
    ACE_Service_Config::open(argc, argv);

    // Instantiate the reactor explicitly.
    ACE_Reactor::instance(new ACE_Reactor(new ACE_TP_Reactor), 1);

    // Start the thread pool.
    ThreadPool * thrpool = new ThreadPool(ACE_Reactor::instance(), 1);
    (void) thrpool;	// FIXME - Clean this up when we are done ...

    PyObject *m, *d;

    // Make sure python threading is turned on.
    PyEval_InitThreads();

    // Setup python thread state association.
    StateMap::initialize();

    // Initialize sub-types in their own modules.
    init_BlockStore();
    init_bsstat();
    init_FileSystem();
    init_PyDirEntryFunc();
    init_stat();
    init_statvfs();

    // Create the module and add the functions.
    m = Py_InitModule("_utp", module_methods);

    // Initialize sub-types which go in this module.
    init_she(m);

    // Add some symbolic constants to the module.
    d = PyModule_GetDict(m);

    ErrorObject =
        PyErr_NewException((char *) "utp.Error", NULL, NULL);
    PyDict_SetItemString(d, "Error", ErrorObject);

    InternalErrorObject =
        PyErr_NewException((char *) "utp.InternalError", ErrorObject, NULL);
    PyDict_SetItemString(d, "InternalError", InternalErrorObject);

    OperationErrorObject =
        PyErr_NewException((char *) "utp.OperationError", ErrorObject, NULL);
    PyDict_SetItemString(d, "OperationError", OperationErrorObject);

    NotFoundErrorObject =
        PyErr_NewException((char *) "utp.NotFoundError", ErrorObject, NULL);
    PyDict_SetItemString(d, "NotFoundError", NotFoundErrorObject);

    NotUniqueErrorObject =
        PyErr_NewException((char *) "utp.NotUniqueError", ErrorObject, NULL);
    PyDict_SetItemString(d, "NotUniqueError", NotUniqueErrorObject);

    ValueErrorObject =
        PyErr_NewException((char *) "utp.ValueError", ErrorObject, NULL);
    PyDict_SetItemString(d, "ValueError", ValueErrorObject);

    ParseErrorObject =
        PyErr_NewException((char *) "utp.ParseError", ErrorObject, NULL);
    PyDict_SetItemString(d, "ParseError", ParseErrorObject);

    VerificationErrorObject =
        PyErr_NewException((char *) "utp.VerificationError", ErrorObject, NULL);
    PyDict_SetItemString(d, "VerificationError", VerificationErrorObject);

    NoSpaceErrorObject =
        PyErr_NewException((char *) "utp.NoSpaceError", ErrorObject, NULL);
    PyDict_SetItemString(d, "NoSpaceError", NoSpaceErrorObject);
}

} // end extern "C"
