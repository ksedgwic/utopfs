#include <memory>
#include <vector>

#include "FileSystemFactory.h"

#include "pyblockstore.h"
#include "pydirentryfunc.h"
#include "pyfilesystem.h"
#include "pystat.h"
#include "pyutpinit.h"

using namespace std;
using namespace utp;

namespace utp {

static PyObject * FileSystemErrorObject;

/* FileSystem methods */

static PyObject *
FileSystem_fs_umount(FileSystemObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":fs_umount"))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_fsh->fs_umount();
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
FileSystem_fs_readlink(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_readlink", &path))
        return NULL;

    char buffer[MAXPATHLEN];

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_readlink(path, buffer, sizeof(buffer));
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    return PyString_FromString(buffer);
}

static PyObject *
FileSystem_fs_mknod(FileSystemObject *self, PyObject *args)
{
    char * path;
    int mode;
    int dev;
    if (!PyArg_ParseTuple(args, "sii:fs_mknod", &path, &mode, &dev))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_mknod(path, mode, dev);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_mkdir(FileSystemObject *self, PyObject *args)
{
    char * path;
    int mode;
    if (!PyArg_ParseTuple(args, "si:fs_mkdir", &path, &mode))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_mkdir(path, mode);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_unlink(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_unlink", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_unlink(path);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_rmdir(FileSystemObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:fs_rmdir", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_rmdir(path);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_symlink(FileSystemObject *self, PyObject *args)
{
    char * opath;
    char * npath;
    if (!PyArg_ParseTuple(args, "ss:fs_symlink", &opath, &npath))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_symlink(opath, npath);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_rename(FileSystemObject *self, PyObject *args)
{
    char * opath;
    char * npath;
    if (!PyArg_ParseTuple(args, "ss:fs_rename", &opath, &npath))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_rename(opath, npath);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_link(FileSystemObject *self, PyObject *args)
{
    char * opath;
    char * npath;
    if (!PyArg_ParseTuple(args, "ss:fs_link", &opath, &npath))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_link(opath, npath);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_chmod(FileSystemObject *self, PyObject *args)
{
    char * path;
    int mode;
    if (!PyArg_ParseTuple(args, "si:fs_chmod", &path, &mode))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_chmod(path, mode);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_truncate(FileSystemObject *self, PyObject *args)
{
    char * path;
    long int size;
    if (!PyArg_ParseTuple(args, "sl:fs_truncate", &path, &size))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_truncate(path, size);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
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
    long int offset = 0;
    
    if (!PyArg_ParseTuple(args, "sl|l:fs_read", &path, &size, &offset))
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

static PyObject *
FileSystem_fs_write(FileSystemObject *self, PyObject *args)
{
    char * path;
    char * data;
    int size;
    long int offset = 0;
    
    if (!PyArg_ParseTuple(args, "ss#|l:fs_write",
                          &path, &data, &size, &offset))
        return NULL;

    int retval;
    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        retval = self->m_fsh->fs_write(path,
                                       data,
                                       size_t(size),
                                       off_t(offset));
    }
    PYUTP_CATCH_ALL;

    return PyInt_FromLong(retval);
}


static PyObject *
FileSystem_fs_readdir(FileSystemObject *self, PyObject *args)
{
    char * path;
    long offset;
    PyObject * cbobj;
    if (!PyArg_ParseTuple(args, "slO:fs_readdir", &path, &offset, &cbobj))
        return NULL;

    PyObject * baseobj = PyObject_GetAttr(cbobj, PyString_FromString("_base"));

    PyDirEntryFuncObject * pdefobj = (PyDirEntryFuncObject *) baseobj;

    // don't want to count this reference, the client is responsible
    // for keeping the callback object around till the end load
    // callback
    Py_DECREF(baseobj);

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_readdir(path, offset, *(pdefobj->decb));
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_access(FileSystemObject *self, PyObject *args)
{
    char * path;
    int mode;
    if (!PyArg_ParseTuple(args, "si:fs_access", &path, &mode))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_access(path, mode);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FileSystem_fs_utime(FileSystemObject *self, PyObject *args)
{
    char * path;
    double atime;
    double mtime;
    if (!PyArg_ParseTuple(args, "sdd:fs_utime", &path, &atime, &mtime))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        errno = - self->m_fsh->fs_utime(path, atime, mtime);
    }
    PYUTP_CATCH_ALL;

    // Convert errno returns to OSError exceptions.
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef FileSystem_methods[] = {
    {"fs_umount",		(PyCFunction)FileSystem_fs_umount,		METH_VARARGS},
    {"fs_getattr",		(PyCFunction)FileSystem_fs_getattr,		METH_VARARGS},
    {"fs_readlink",		(PyCFunction)FileSystem_fs_readlink,	METH_VARARGS},
    {"fs_mknod",		(PyCFunction)FileSystem_fs_mknod,		METH_VARARGS},
    {"fs_mkdir",		(PyCFunction)FileSystem_fs_mkdir,		METH_VARARGS},
    {"fs_unlink",		(PyCFunction)FileSystem_fs_unlink,		METH_VARARGS},
    {"fs_rmdir",		(PyCFunction)FileSystem_fs_rmdir,		METH_VARARGS},
    {"fs_symlink",		(PyCFunction)FileSystem_fs_symlink,		METH_VARARGS},
    {"fs_rename",		(PyCFunction)FileSystem_fs_rename,		METH_VARARGS},
    {"fs_link",			(PyCFunction)FileSystem_fs_link,		METH_VARARGS},
    {"fs_chmod",		(PyCFunction)FileSystem_fs_chmod,		METH_VARARGS},
    {"fs_truncate",		(PyCFunction)FileSystem_fs_truncate,	METH_VARARGS},
    {"fs_open",			(PyCFunction)FileSystem_fs_open,		METH_VARARGS},
    {"fs_read",			(PyCFunction)FileSystem_fs_read,		METH_VARARGS},
    {"fs_write",		(PyCFunction)FileSystem_fs_write,		METH_VARARGS},
    {"fs_readdir",		(PyCFunction)FileSystem_fs_readdir,		METH_VARARGS},
    {"fs_access",		(PyCFunction)FileSystem_fs_access,		METH_VARARGS},
    {"fs_utime",		(PyCFunction)FileSystem_fs_utime,		METH_VARARGS},
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
FileSystemModule_mkfs(PyObject *self, PyObject *i_args)
{
    char * name;
    char * fsid;
    char * pass;
    PyObject * bsobj;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "sO!ssO!:mkfs",
                          &name,
                          &BlockStore_Type, &bsobj,
                          &fsid, &pass,
                          &PyTuple_Type, &tup))
		return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "FileSystem::mkfs arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    BlockStoreObject * bsop = (BlockStoreObject *) bsobj;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkFileSystemObject(FileSystemFactory::mkfs(name,
                                                          bsop->m_bsh,
                                                          fsid,
                                                          pass,
                                                          args));
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
FileSystemModule_mount(PyObject *self, PyObject *i_args)
{
    char * name;
    char * fsid;
    char * pass;
    PyObject * bsobj;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "sO!ssO!:mount",
                          &name,
                          &BlockStore_Type, &bsobj,
                          &fsid, &pass,
                          &PyTuple_Type, &tup))
		return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "FileSystem::mount arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    BlockStoreObject * bsop = (BlockStoreObject *) bsobj;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkFileSystemObject(FileSystemFactory::mount(name,
                                                           bsop->m_bsh,
                                                           fsid,
                                                           pass,
                                                           args));
    }
    PYUTP_CATCH_ALL;
}

static PyMethodDef FileSystemModule_methods[] = {
	{"mkfs",			FileSystemModule_mkfs,			METH_VARARGS},
	{"mount",			FileSystemModule_mount,			METH_VARARGS},
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
