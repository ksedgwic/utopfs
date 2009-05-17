#ifndef pystat_h__
#define pystat_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PyTypeObject StatResultType;

#define StatObject_Check(v)	((v)->ob_type == &StatResultType)

PyObject * pystat_fromstructstat(struct stat const * st);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_stat(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pystat_h__
