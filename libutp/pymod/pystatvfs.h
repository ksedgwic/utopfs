#ifndef pystatvfs_h__
#define pystatvfs_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PyTypeObject StatVFSResultType;

#define StatVFSObject_Check(v)	\
    ((v)->ob_type == &StatVFSResultType)

PyObject * pystatvfs_fromstructstatvfs(struct statvfs const * stvbuf);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_statvfs(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pystatvfs_h__
