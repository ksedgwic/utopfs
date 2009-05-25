#include <memory>

#include "pyutpinit.h"
#include "pyblockstore.h"

using namespace utp;

namespace utp {

static PyObject * BlockStoreErrorObject;

/* BlockStore methods */

static PyObject *
BlockStore_bs_create(BlockStoreObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:bs_create", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_create(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_open(BlockStoreObject *self, PyObject *args)
{
    char * path;
    if (!PyArg_ParseTuple(args, "s:bs_open", &path))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_open(path);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_close(BlockStoreObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":bs_close"))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_close();
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_get_block(BlockStoreObject *self, PyObject *args)
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
        outsz = self->m_bsh->bs_get_block(keyptr, keylen,
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
BlockStore_bs_put_block(BlockStoreObject *self, PyObject *args)
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
        self->m_bsh->bs_put_block(keyptr, keylen, valptr, vallen);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_del_block(BlockStoreObject *self, PyObject *args)
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
        self->m_bsh->bs_del_block(keyptr, keylen);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyMethodDef BlockStore_methods[] = {
    {"bs_create",		(PyCFunction)BlockStore_bs_create,		METH_VARARGS},
    {"bs_open",			(PyCFunction)BlockStore_bs_open,		METH_VARARGS},
    {"bs_close",		(PyCFunction)BlockStore_bs_close,		METH_VARARGS},
    {"bs_get_block",	(PyCFunction)BlockStore_bs_get_block,	METH_VARARGS},
    {"bs_put_block",	(PyCFunction)BlockStore_bs_put_block,	METH_VARARGS},
    {"bs_del_block",	(PyCFunction)BlockStore_bs_del_block,	METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};

static void
BlockStore_dealloc(BlockStoreObject *self)
{
    self->m_bsh.~BlockStoreHandle();  // Destructs, doesn't free handle).
    PyObject_Del(self);
}

static PyObject *
BlockStore_getattr(BlockStoreObject *self, char *name)
{
    return Py_FindMethod(BlockStore_methods, (PyObject *)self, name);
}

PyTypeObject BlockStore_Type = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyObject_HEAD_INIT(NULL)
    0,										/*ob_size*/
    "utp.BlockStore",						/*tp_name*/
    sizeof(BlockStoreObject),				/*tp_basicsize*/
    0,										/*tp_itemsize*/
    /* methods */
    (destructor)BlockStore_dealloc, 		/*tp_dealloc*/
    0,										/*tp_pr*/
    (getattrfunc)BlockStore_getattr, 		/*tp_getattr*/
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
mkBlockStoreObject(BlockStoreHandle const & i_nmh)
{
    // Is this a nil handle?
    if (!i_nmh)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    BlockStoreObject * self = PyObject_New(BlockStoreObject, &BlockStore_Type);
    if (self != NULL)
    {
        // "placement new", see Stroustrup 10.4.11
        (void) new(static_cast<void *>(&self->m_bsh)) BlockStoreHandle(i_nmh);
    }

    return reinterpret_cast<PyObject *>(self);
}

//----------------------------------------------------------------
// Module Methods
//----------------------------------------------------------------

static PyObject *
BlockStoreModule_instance(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":instance"))
		return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkBlockStoreObject(BlockStore::instance());
    }
    PYUTP_CATCH_ALL;
}

static PyMethodDef BlockStoreModule_methods[] = {
	{"instance",		BlockStoreModule_instance,			METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_BlockStore(void)
{
	PyObject *m, *d;

	/* Initialize the type of the new type object here; doing it here
	 * is required for portability to Windows without requiring C++. */
	BlockStore_Type.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule("_BlockStore", BlockStoreModule_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	BlockStoreErrorObject =
        PyErr_NewException("BlockStore.error", NULL, NULL);
	PyDict_SetItemString(d, "error", BlockStoreErrorObject);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End: