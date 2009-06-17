#include <string>

#include "pyutpinit.h"
#include "pydirentryfunc.h"
#include "statemap.h"
#include "pystat.h"

using namespace std;

namespace utp {

PyDirEntryFunc::PyDirEntryFunc(PyObject * cbobj)
    : m_cbobj(cbobj)
{
    // IMPORTANT - this constructor must be called with the
    // interperter lock held and the threadstate set.

    // note our interperter, for bind checking later
    m_interp = PyThreadState_Get()->interp;
}

PyDirEntryFunc::~PyDirEntryFunc()
{
}

bool
PyDirEntryFunc::def_entry(string const & i_name,
                          struct stat const * i_stbuf,
                          off_t i_off)
{
    PyThreadState * tstate = StateMap::associate(m_interp);
    PyEval_AcquireThread(tstate);

    PyObject * stobj = i_stbuf ? pystat_fromstructstat(i_stbuf) : Py_None;
    Py_INCREF(stobj);

    PyObject * method = PyObject_GetAttrString(m_cbobj, "def_entry");
    PyObject * args = Py_BuildValue("(sOl)", i_name.c_str(), stobj, i_off);
    PyObject * result = PyEval_CallObject(method, args);


    // FIXME - figure out how to translate exceptions.
    
    Py_XDECREF(result);
    Py_DECREF(args);
    Py_DECREF(method);

    PyEval_ReleaseThread(tstate);

    // FIXME - need to return a real value here!
    return 0;
}

static PyObject * PyDirEntryFuncErrorObject;

static PyMethodDef PyDirEntryFunc_methods[] = {
    {NULL,		NULL}		/* sentinel */
};

static void
PyDirEntryFunc_dealloc(PyDirEntryFuncObject *self)
{
    delete self->decb;
    PyObject_Del(self);
}

static PyObject *
PyDirEntryFunc_getattr(PyDirEntryFuncObject *self, char *name)
{
    return Py_FindMethod(PyDirEntryFunc_methods, (PyObject *)self, name);
}

PyTypeObject PyDirEntryFunc_Type = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyObject_HEAD_INIT(NULL)
    0,										/*ob_size*/
    "utp.PyDirEntryFunc",					/*tp_name*/
    sizeof(PyDirEntryFuncObject),			/*tp_basicsize*/
    0,										/*tp_itemsize*/
    /* methods */
    (destructor)PyDirEntryFunc_dealloc, 	/*tp_dealloc*/
    0,										/*tp_pr*/
    (getattrfunc)PyDirEntryFunc_getattr,	/*tp_getattr*/
    0,					 					/*tp_setattr*/
    0,										/*tp_compare*/
    0,										/*tp_repr*/
    0,										/*tp_as_number*/
    0,										/*tp_as_sequence*/
    0,										/*tp_as_mapping*/
    0,										/*tp_hash*/
    0,                      				/*tp_call*/
    0,                      				/*tp_str*/
    0,                      				/*tp_getattro*/
    0,                      				/*tp_setattro*/
    0,                      				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     				/*tp_flags*/
    0,                      				/*tp_doc*/
    0,                      				/*tp_traverse*/
    0,                      				/*tp_clear*/
    0,                      				/*tp_richcompare*/
    0,                      				/*tp_weaklistoffset*/
    0,                      				/*tp_iter*/
    0,                      				/*tp_iternext*/
    0,                      				/*tp_methods*/
    0,                      				/*tp_members*/
    0,                      				/*tp_getset*/
    0,                      				/*tp_base*/
    0,                      				/*tp_dict*/
    0,                      				/*tp_descr_get*/
    0,                      				/*tp_descr_set*/
    0,                      				/*tp_dictoffset*/
    0,                      				/*tp_init*/
    0,                      				/*tp_alloc*/
    0,                      				/*tp_new*/
    0,                      				/*tp_free*/
    0,                      				/*tp_is_gc*/
};

PyObject *
mkPyDirEntryFuncObject(PyDirEntryFunc * pdef)
{
    PyDirEntryFuncObject * self =
        PyObject_New(PyDirEntryFuncObject, &PyDirEntryFunc_Type);

    if (self != NULL)
        self->decb = pdef;

    return reinterpret_cast<PyObject *>(self);
}

static PyDirEntryFuncObject *
newPyDirEntryFuncObject(PyObject * args, PyObject * wrapobj)
{
    return reinterpret_cast<PyDirEntryFuncObject *>
        (mkPyDirEntryFuncObject(new PyDirEntryFunc(wrapobj)));
}

//----------------------------------------------------------------
// Module Methods
//----------------------------------------------------------------

static PyObject *
PyDirEntryFuncModule_new(PyObject *self, PyObject *args)
{
    PyObject * wrapobj;
	if (!PyArg_ParseTuple(args, "O:new", &wrapobj))
		return NULL;

    return reinterpret_cast<PyObject *>
        (newPyDirEntryFuncObject(args, wrapobj));
}

static PyMethodDef PyDirEntryFuncModule_methods[] = {
	{"new",		PyDirEntryFuncModule_new,		METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_PyDirEntryFunc(void)
{
	PyObject *m, *d;

	/* Initialize the type of the new type object here; doing it here
	 * is required for portability to Windows without requiring C++. */
	PyDirEntryFunc_Type.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule("_PyDirEntryFunc", PyDirEntryFuncModule_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	PyDirEntryFuncErrorObject =
        PyErr_NewException((char *) "PyDirEntryFunc.error", NULL, NULL);
	PyDict_SetItemString(d, "error", PyDirEntryFuncErrorObject);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
