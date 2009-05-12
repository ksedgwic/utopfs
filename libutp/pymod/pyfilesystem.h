#ifndef pyfilesystem_h__
#define pyfilesystem_h__

#include "FileSystem.h"

#if defined(LINUX)
// Python and sys/features clash over this.
# undef  _POSIX_C_SOURCE
#endif

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

typedef struct {
    PyObject_HEAD
    FileSystemHandle m_fsh;
} FileSystemObject;

extern PyTypeObject FileSystem_Type;

#define FileSystemObject_Check(v)	((v)->ob_type == &FileSystem_Type)

// Increases the refcount of the object.
PyObject * mkFileSystemObject(FileSystemHandle const & i_fsh);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_FileSystem(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pyfilesystem_h__
