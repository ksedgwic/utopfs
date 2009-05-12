#include <memory>

#include "pyutpinit.h"
#include "pyfilesystem.h"

using namespace utp;

namespace utp {

static PyObject * FileSystemErrorObject;

/* FileSystem methods */

#if 0
static PyObject *
FileSystem_bs_create(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:bs_create", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->bs_create(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_bs_open(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:bs_open", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->bs_open(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_bs_close(FileSystemObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":bs_close"))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->bs_close();
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_bs_get_block(FileSystemObject *self, PyObject *args)
{
    PyObject * keyobj;
    if (!PyArg_ParseTuple(args, "O!:bs_get_block",
                          &PyBuffer_Type, &keyobj))
        return NULL;

    void const * keyptr;
    Py_ssize_t keylen;
    if (PyObject_AsReadBuffer(keyobj, &keyptr, &keylen))
        return NULL;

    char outbuf[32 * 1024];
    size_t outsz;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        outsz = self->m_fsh->bs_get_block(keyptr, keylen,
                                          outbuf, sizeof(outbuf));
    }
    PYUTP_CATCH_ALL;

    PyObject * retobj = PyBuffer_New(outsz);
    void * outptr;
    Py_ssize_t outlen;
    if (PyObject_AsWriteBuffer(retobj, &outptr, &outlen))
        return NULL;

    memcpy(outptr, outbuf, outlen);
    return retobj;
}

static PyObject *
FileSystem_bs_put_block(FileSystemObject *self, PyObject *args)
{
    PyObject * keyobj;
    PyObject * valobj;
    if (!PyArg_ParseTuple(args, "O!O!:bs_put_block",
                          &PyBuffer_Type, &keyobj,
                          &PyBuffer_Type, &valobj))
        return NULL;

    void const * keyptr;
    Py_ssize_t keylen;
    if (PyObject_AsReadBuffer(keyobj, &keyptr, &keylen))
        return NULL;

    void const * valptr;
    Py_ssize_t vallen;
    if (PyObject_AsReadBuffer(valobj, &valptr, &vallen))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->bs_put_block(keyptr, keylen, valptr, vallen);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_bs_del_block(FileSystemObject *self, PyObject *args)
{
    PyObject * keyobj;
    if (!PyArg_ParseTuple(args, "O!:bs_del_block",
                          &PyBuffer_Type, &keyobj))
        return NULL;

    void const * keyptr;
    Py_ssize_t keylen;
    if (PyObject_AsReadBuffer(keyobj, &keyptr, &keylen))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->bs_del_block(keyptr, keylen);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}
#endif

static PyMethodDef FileSystem_methods[] = {
#if 0
    {"bs_create",		(PyCFunction)FileSystem_bs_create,		METH_VARARGS},
    {"bs_open",			(PyCFunction)FileSystem_bs_open,		METH_VARARGS},
    {"bs_close",		(PyCFunction)FileSystem_bs_close,		METH_VARARGS},
    {"bs_get_block",	(PyCFunction)FileSystem_bs_get_block,	METH_VARARGS},
    {"bs_put_block",	(PyCFunction)FileSystem_bs_put_block,	METH_VARARGS},
    {"bs_del_block",	(PyCFunction)FileSystem_bs_del_block,	METH_VARARGS},
#endif
    {NULL,		NULL}		/* sentinel */
};

static void
FileSystem_dealloc(FileSystemObject *self)
{
    self->m_fsh.~FileSystemHandle();  // Destructs, doesn't free handle).
    PyObject_Del(self);
}

static PyObject *
FileSystem_getattr(FileSystemObject *self, char *name)
{
    return Py_FindMethod(FileSystem_methods, (PyObject *)self, name);
}

PyTypeObject FileSystem_Type = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyObject_HEAD_INIT(NULL)
    0,										/*ob_size*/
    "utp.FileSystem",						/*tp_name*/
    sizeof(FileSystemObject),				/*tp_basicsize*/
    0,										/*tp_itemsize*/
    /* methods */
    (destructor)FileSystem_dealloc, 		/*tp_dealloc*/
    0,										/*tp_pr*/
    (getattrfunc)FileSystem_getattr, 		/*tp_getattr*/
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
mkFileSystemObject(FileSystemHandle const & i_nmh)
{
    // Is this a nil handle?
    if (!i_nmh)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    FileSystemObject * self = PyObject_New(FileSystemObject, &FileSystem_Type);
    if (self != NULL)
    {
        // "placement new", see Stroustrup 10.4.11
        (void) new(static_cast<void *>(&self->m_fsh)) FileSystemHandle(i_nmh);
    }

    return reinterpret_cast<PyObject *>(self);
}

//----------------------------------------------------------------
// Module Methods
//----------------------------------------------------------------

static PyObject *
FileSystemModule_instance(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":instance"))
		return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkFileSystemObject(FileSystem::instance());
    }
    PYUTP_CATCH_ALL;
}

static PyMethodDef FileSystemModule_methods[] = {
	{"instance",		FileSystemModule_instance,			METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_FileSystem(void)
{
	PyObject *m, *d;

	/* Initialize the type of the new type object here; doing it here
	 * is required for portability to Windows without requiring C++. */
	FileSystem_Type.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule("_FileSystem", FileSystemModule_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	FileSystemErrorObject =
        PyErr_NewException("FileSystem.error", NULL, NULL);
	PyDict_SetItemString(d, "error", FileSystemErrorObject);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
