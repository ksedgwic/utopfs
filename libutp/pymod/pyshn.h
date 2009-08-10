#ifndef pyshn_h__
#define pyshn_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PyTypeObject SignedHeadNodeType;

#define SignedHeadNodeObject_Check(v) \
    ((v)->ob_type == &SignedHeadNodeType)

PyObject * pyshn_fromprotoshn(SignedHeadNode const & i_shn);

void pyshn_asprotoshn(PyObject * shnobj, SignedHeadNode & shnref);

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_shn(PyObject * m);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pyshn_h__
