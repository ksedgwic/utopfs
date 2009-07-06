#ifndef pybsstat_h__
#define pybsstat_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

#include "BlockStore.h"

namespace utp {

extern PyTypeObject BlockStoreStatResultType;

#define BlockStoreStatObject_Check(v)	\
    ((v)->ob_type == &BlockStoreStatResultType)

PyObject * pybsstat_fromstructbsstat(struct BlockStore::Stat const * bsst);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_bsstat(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pybsstat_h__
