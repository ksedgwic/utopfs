#ifndef statemap_h__
#define statemap_h__

#include <map>

#include <ace/Synch.h>
#include <ace/TSS_T.h>

#if defined(WIN32)
// Python and ACE clash providing WIN32's missing pid_t
#define pid_t __pid_t
#endif

// Unfortunately both python and the system defined this
#undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

struct ltbind {
bool operator()(PyInterpreterState * p1,
                PyInterpreterState * p2) const
    {
        return ((void *) p1 < (void *) p2) ? 1 : 0;
    }
};

class PyBindingMap : public std::map<PyInterpreterState *,
                                     PyThreadState *,
                                     ltbind>
{
public:
    // default constructor will be fine ...
    PyBindingMap() {}

    // clear and delete any bindings on destruction ...
    ~PyBindingMap()
    {
        PyEval_AcquireLock();
        PyBindingMap::iterator i;
        for (i = this->begin(); i != this->end(); i++) {
            PyThreadState * tstate = i->second;
            PyThreadState_Clear(tstate);
            PyThreadState_Delete(tstate);
        }
        PyEval_ReleaseLock();
    }
    };

class StateMap
{
public:
    static void initialize();
    static PyThreadState * associate(PyInterpreterState * interp);
private:
    static ACE_TSS<PyBindingMap> g_mapp;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// End:

#endif //  statemap_h__
