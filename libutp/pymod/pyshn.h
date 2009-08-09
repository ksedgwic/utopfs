#ifndef pyshn_h__
#define pyshn_h__

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

extern PyTypeObject SignedHeadNodeResultType;

#define SignedHeadNodeObject_Check(v)	\
    ((v)->ob_type == &SignedHeadNodeResultType)

PyObject * pyshn_fromprotoshn(SignedHeadNode const & i_shn);

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
