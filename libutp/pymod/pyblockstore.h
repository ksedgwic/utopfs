#ifndef pyblockstore_h__
#define pyblockstore_h__

#include "BlockStore.h"

#if defined(LINUX)
// Python and sys/features clash over this.
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
#endif

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

typedef struct {
    PyObject_HEAD
    BlockStoreHandle m_bsh;
} BlockStoreObject;

extern PyTypeObject BlockStore_Type;

#define BlockStoreObject_Check(v)	((v)->ob_type == &BlockStore_Type)

// Increases the refcount of the object.
PyObject * mkBlockStoreObject(BlockStoreHandle const & i_bsh);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_BlockStore(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pyblockstore_h__
