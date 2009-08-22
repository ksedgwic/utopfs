#ifndef pyshe_h__
#define pyshe_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PyTypeObject SignedHeadEdgeType;

#define SignedHeadEdgeObject_Check(v) \
    ((v)->ob_type == &SignedHeadEdgeType)

PyObject * pyshe_fromprotoshe(SignedHeadEdge const & i_she);

void pyshe_asprotoshe(PyObject * sheobj, SignedHeadEdge & sheref);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_she(PyObject * m);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pyshe_h__
