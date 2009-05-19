#ifndef pydirentryfunc_h__
#define pydirentryfunc_h__

#include <string>

#include "FileSystem.h"

#include <Python.h>             // CONFLICT: include this after ACE includes

namespace utp {

class PyDirEntryFunc : public FileSystem::DirEntryFunc
{
public:
    PyDirEntryFunc(PyObject * cbobj);

    virtual ~PyDirEntryFunc();

    virtual bool def_entry(std::string const & i_name,
                           struct stat const * i_stbuf,
                           off_t i_off);
protected:
    PyInterpreterState *	m_interp;
    PyObject *				m_cbobj;
};

typedef struct {
	PyObject_HEAD
    PyDirEntryFunc * decb;
} PyDirEntryFuncObject;

extern PyTypeObject PyDirEntryFunc_Type;

#define PyDirEntryFuncObject_Check(v)	((v)->ob_type == &PyDirEntryFunc_Type)

#if defined(WIN32)
    __declspec( dllexport )
#endif
void init_PyDirEntryFunc(void);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif //  pydirentryfunc_h__
