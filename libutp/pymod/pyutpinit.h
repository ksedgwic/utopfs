#ifndef initpy_h__
#define initpy_h__

#include <exception>

#include "Except.h"

#include "pyutpexp.h"

#if defined(WIN32)
// Python and ACE clash providing WIN32's missing pid_t
#define pid_t __pid_t
#endif

#if defined(LINUX)
// Python and sys/features clash over this.
# undef  _POSIX_C_SOURCE
#endif

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PYUTP_EXP PyObject * ErrorObject;

extern PYUTP_EXP PyObject * InternalErrorObject;
extern PYUTP_EXP PyObject * OperationErrorObject;
extern PYUTP_EXP PyObject * NotFoundErrorObject;
extern PYUTP_EXP PyObject * NotUniqueErrorObject;
extern PYUTP_EXP PyObject * ValueErrorObject;
extern PYUTP_EXP PyObject * ParseErrorObject;
    
extern PYUTP_EXP void PyError_FromException(std::exception const & ex);

#define PYUTP_TRY                               \
    do {                                        \
    try {

#define PYUTP_CATCH_ALL                         \
    }                                           \
        catch(std::exception const & ex) {      \
            utp::PyError_FromException(ex);     \
            return NULL;                        \
        }                                       \
} while (false);

    class PYUTP_THREADED_SCOPE {
    public:
        PYUTP_THREADED_SCOPE() { Py_UNBLOCK_THREADS }
        ~PYUTP_THREADED_SCOPE() { Py_BLOCK_THREADS }
    protected:
        PyThreadState * _save;
    };

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // initpy_h__
