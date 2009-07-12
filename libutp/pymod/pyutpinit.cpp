#include <typeinfo>

#include <ace/Service_Config.h>

#include "pyutpinit.h"
#include "statemap.h"

#include "pyblockstore.h"
#include "pybsstat.h"
#include "pydirentryfunc.h"
#include "pyfilesystem.h"
#include "pystat.h"
#include "pystatvfs.h"

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

    PyObject *m, *d;

    // make sure python threading is turned on
    PyEval_InitThreads();

    // setup python thread state association
    StateMap::initialize();

    // initialize sub-types
    init_stat();
    init_statvfs();
    init_bsstat();
    init_BlockStore();
    init_FileSystem();
    init_PyDirEntryFunc();

    // create the module and add the functions
    m = Py_InitModule("_utp", module_methods);

    // add some symbolic constants to the module
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
