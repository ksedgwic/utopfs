#include <memory>
#include <vector>

#include "pyutpinit.h"
#include "pyfilesystem.h"
#include "pystat.h"

using namespace std;
using namespace utp;

namespace utp {

static PyObject * FileSystemErrorObject;

/* FileSystem methods */

static PyObject *
FileSystem_fs_mkfs(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_mkfs", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->fs_mkfs(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_fs_mount(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_mount", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->fs_mount(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystem_fs_getattr(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_getattr", &path))
        return NULL;

    struct stat statbuf;
    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_getattr(path, &statbuf);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    return pystat_fromstructstat(&statbuf);
}

static PyObject *
FileSystem_fs_open(FileSystemObject *self, PyObject *args)
{
    char * path;
    int flags;
    if (!PyArg_ParseTuple(args, "si:fs_open", &path, &flags))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_open(path, flags);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_read(FileSystemObject *self, PyObject *args)
{
    char * path;
    long int size;
    long int offset;
    
    if (!PyArg_ParseTuple(args, "sll:fs_read", &path, &size, &offset))
        return NULL;

    // Allocate a buffer big enough for the users entire request.
    vector<unsigned char> buffer(size);

    int retval;
    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        retval = self->m_fsh->fs_read(path,
                                      &buffer[0],
                                      size_t(size),
                                      off_t(offset));
    }
    PYUTP_CATCH_ALL;

    // Was there an error?
    if (retval < 0)
    {
        errno = -retval;
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    // Create a buffer object which is exactly as big as the
    // returned data.
    //
    PyObject * retobj = PyBuffer_New(retval);
    void * outptr;
    Py_ssize_t outlen;
    if (PyObject_AsWriteBuffer(retobj, &outptr, &outlen))
        return NULL;

    memcpy(outptr, &buffer[0], outlen);

    return retobj;
}

static PyMethodDef FileSystem_methods[] = {
    {"fs_mkfs",			(PyCFunction)FileSystem_fs_mkfs,		METH_VARARGS},
    {"fs_mount",		(PyCFunction)FileSystem_fs_mount,		METH_VARARGS},
    {"fs_getattr",		(PyCFunction)FileSystem_fs_getattr,		METH_VARARGS},
    {"fs_open",			(PyCFunction)FileSystem_fs_open,		METH_VARARGS},
    {"fs_read",			(PyCFunction)FileSystem_fs_read,		METH_VARARGS},
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
