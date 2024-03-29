#include <memory>
#include <string>

#include "Types.h"
#include "BlockStoreFactory.h"

#include "pyblockstore.h"
#include "pybsstat.h"
#include "pyshe.h"
#include "pyutpinit.h"

using namespace std;
using namespace utp;

namespace utp {

static PyObject * BlockStoreErrorObject;

/* BlockStore methods */

static PyObject *
BlockStore_bs_create(BlockStoreObject * self, PyObject * i_args)
{
    unsigned long size;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "kO!:bs_create", &size, &PyTuple_Type, &tup))
        return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_create(size, args);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_open(BlockStoreObject * self, PyObject * i_args)
{
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "s:bs_open", &PyTuple_Type, &tup))
        return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_open(args);
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
BlockStore_bs_stat(BlockStoreObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":bs_stat"))
        return NULL;

    BlockStore::Stat bsstat;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_stat(bsstat);
    }
    PYUTP_CATCH_ALL;

    return pybsstat_fromstructbsstat(&bsstat);
}

static PyObject *
BlockStore_bs_sync(BlockStoreObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":bs_sync"))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_sync();
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_block_get(BlockStoreObject *self, PyObject *args)
{
    PyObject * keyobj;
    if (!PyArg_ParseTuple(args, "O!:bs_block_get",
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
        outsz = self->m_bsh->bs_block_get(keyptr, keylen,
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
BlockStore_bs_block_put(BlockStoreObject *self, PyObject *args)
{
    PyObject * keyobj;
    PyObject * valobj;
    if (!PyArg_ParseTuple(args, "O!O!:bs_block_put",
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
        self->m_bsh->bs_block_put(keyptr, keylen, valptr, vallen);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_refresh_start(BlockStoreObject *self, PyObject *args)
{
    uint64 rid;
    if (!PyArg_ParseTuple(args, "K:bs_refresh_start", &rid))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_refresh_start(rid);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_refresh_blocks(BlockStoreObject *self, PyObject *args)
{
    PyObject * keysobj;
    uint64 rid;
    if (!PyArg_ParseTuple(args, "KO:bs_refresh_blocks", &rid, &keysobj))
        return NULL;

    // Translate the python sequence arguent into a KeySeq.
    BlockStore::KeySeq keys;
    if (!PySequence_Check(keysobj))
    {
        PyErr_SetString(PyExc_TypeError,
                        "bs_refresh_blocks arg must be sequence");
        return NULL;
    }
    for (Py_ssize_t i = 0; i < PySequence_Size(keysobj); ++i)
    {
        PyObject * ko = PySequence_GetItem(keysobj, i);
        void const * keyptr;
        Py_ssize_t keylen;
        if (PyObject_AsReadBuffer(ko, &keyptr, &keylen))
            return NULL;

        uint8 const * kb = (uint8 *) keyptr;
        uint8 const * ke = kb + keylen;
        keys.push_back(OctetSeq(kb, ke));
    }

    BlockStore::KeySeq missing;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_refresh_blocks(rid, keys, missing);
    }
    PYUTP_CATCH_ALL;

    // Translate the returned missing KeySeq into a python list.
    PyObject * missobj = PyList_New(missing.size());
    for (unsigned i = 0; i < missing.size(); ++i)
    {
        PyObject * mobj = PyBuffer_New(missing[i].size());
        void * outptr;
        Py_ssize_t outlen;
        if (PyObject_AsWriteBuffer(mobj, &outptr, &outlen))
            return NULL;
        memcpy(outptr, &missing[i][0], outlen);
        PyList_SetItem(missobj, i, mobj);
    }

    return missobj;
}

static PyObject *
BlockStore_bs_refresh_finish(BlockStoreObject *self, PyObject *args)
{
    uint64 rid;
    if (!PyArg_ParseTuple(args, "K:bs_refresh_finish", &rid))
        return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_refresh_finish(rid);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_head_insert(BlockStoreObject *self, PyObject *args)
{
    PyObject * sheobj;
    if (!PyArg_ParseTuple(args, "O!:bs_head_insert",
                          &SignedHeadEdgeType, &sheobj))
        return NULL;

    SignedHeadEdge she;
    pyshe_asprotoshe(sheobj, she);

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_head_insert(she);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStore_bs_head_follow(BlockStoreObject *self, PyObject *args)
{
    PyObject * hnobj;
    if (!PyArg_ParseTuple(args, "O!:bs_head_follow", &PyTuple_Type, &hnobj))
        return NULL;

    if (PyTuple_Size(hnobj) != 2)
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_follow "
                        "arg must be tuplea of two buffers");
            return NULL;
    }
    PyObject * tagbuf = PyTuple_GetItem(hnobj, 0);
    if (!PyBuffer_Check(tagbuf))
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_follow "
                        "arg must be tuple of two buffers");
        return NULL;
    }
    char const * tagptr;
    Py_ssize_t taglen;
    if (PyObject_AsReadBuffer(tagbuf, (void const **) &tagptr, &taglen))
        return NULL;

    PyObject * refbuf = PyTuple_GetItem(hnobj, 1);
    if (!PyBuffer_Check(refbuf))
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_follow "
                        "arg must be tuple of two buffers");
        return NULL;
    }
    char const * refptr;
    Py_ssize_t reflen;
    if (PyObject_AsReadBuffer(refbuf, (void const **) &refptr, &reflen))
        return NULL;

    HeadNode hn = make_pair(string(tagptr, tagptr + taglen),
                            string(refptr, refptr + reflen));

    BlockStore::SignedHeadEdgeSeq shes;
    
    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_head_follow(hn, shes);
    }
    PYUTP_CATCH_ALL;

    // Translate the returned SignedHeadEdgeSeq into a python list.
    PyObject * shesobj = PyList_New(shes.size());
    for (unsigned i = 0; i < shes.size(); ++i)
    {
        PyObject * so = pyshe_fromprotoshe(shes[i]);
        if (!so)
        {
            Py_DECREF(shesobj);
            return NULL;
        }
        PyList_SetItem(shesobj, i, so);
    }

    return shesobj;
}

static PyObject *
BlockStore_bs_head_furthest(BlockStoreObject *self, PyObject *args)
{
    PyObject * hnobj;
    if (!PyArg_ParseTuple(args, "O!:bs_head_furthest", &PyTuple_Type, &hnobj))
        return NULL;

    if (PyTuple_Size(hnobj) != 2)
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_furthest "
                        "arg must be tuplea of two buffers");
            return NULL;
    }
    PyObject * tagbuf = PyTuple_GetItem(hnobj, 0);
    if (!PyBuffer_Check(tagbuf))
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_furthest "
                        "arg must be tuple of two buffers");
        return NULL;
    }
    char const * tagptr;
    Py_ssize_t taglen;
    if (PyObject_AsReadBuffer(tagbuf, (void const **) &tagptr, &taglen))
        return NULL;

    PyObject * refbuf = PyTuple_GetItem(hnobj, 1);
    if (!PyBuffer_Check(refbuf))
    {
        PyErr_SetString(PyExc_TypeError,
                        "BlockStore::bs_head_furthest "
                        "arg must be tuple of two buffers");
        return NULL;
    }
    char const * refptr;
    Py_ssize_t reflen;
    if (PyObject_AsReadBuffer(refbuf, (void const **) &refptr, &reflen))
        return NULL;

    HeadNode hn = make_pair(string(tagptr, tagptr + taglen),
                            string(refptr, refptr + reflen));

    HeadNodeSeq hns;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        self->m_bsh->bs_head_furthest(hn, hns);
    }
    PYUTP_CATCH_ALL;

    // Translate the returned HeadNodeSeq into a python list.
    PyObject * hnsobj = PyList_New(hns.size());
    for (unsigned i = 0; i < hns.size(); ++i)
    {
        void * optr;
        Py_ssize_t olen;

        PyObject * tagobj = PyBuffer_New(hns[i].first.size());
        if (!tagobj)
        {
            Py_DECREF(hnsobj);
            return NULL;
        }
        if (PyObject_AsWriteBuffer(tagobj, &optr, &olen))
        {
            Py_DECREF(tagobj);
            Py_DECREF(hnsobj);
            return NULL;
        }
        memcpy(optr, hns[i].first.data(),
               min(hns[i].first.size(), size_t(olen)));

        PyObject * refobj = PyBuffer_New(hns[i].second.size());
        if (!refobj)
        {
            Py_DECREF(tagobj);
            Py_DECREF(hnsobj);
            return NULL;
        }
        if (PyObject_AsWriteBuffer(refobj, &optr, &olen))
        {
            Py_DECREF(refobj);
            Py_DECREF(tagobj);
            Py_DECREF(hnsobj);
            return NULL;
        }
        memcpy(optr, hns[i].second.data(),
               min(hns[i].second.size(), size_t(olen)));

        PyObject * tup = PyTuple_New(2);
        if (!tup)
        {
            Py_DECREF(refobj);
            Py_DECREF(tagobj);
            Py_DECREF(hnsobj);
            return NULL;
        }
        PyTuple_SetItem(tup, 0, tagobj);
        PyTuple_SetItem(tup, 1, refobj);
        PyList_SetItem(hnsobj, i, tup);
    }

    return hnsobj;
}

static PyMethodDef BlockStore_methods[] = {
    {"bs_create",		(PyCFunction)BlockStore_bs_create,		METH_VARARGS},
    {"bs_open",			(PyCFunction)BlockStore_bs_open,		METH_VARARGS},
    {"bs_close",		(PyCFunction)BlockStore_bs_close,		METH_VARARGS},
    {"bs_stat",			(PyCFunction)BlockStore_bs_stat,		METH_VARARGS},
    {"bs_sync",			(PyCFunction)BlockStore_bs_sync,		METH_VARARGS},
    {"bs_block_get",	(PyCFunction)BlockStore_bs_block_get,	METH_VARARGS},
    {"bs_block_put",	(PyCFunction)BlockStore_bs_block_put,	METH_VARARGS},
    {"bs_refresh_start",
     				(PyCFunction)BlockStore_bs_refresh_start,	METH_VARARGS},
    {"bs_refresh_blocks",
					(PyCFunction)BlockStore_bs_refresh_blocks,	METH_VARARGS},
    {"bs_refresh_finish",
     				(PyCFunction)BlockStore_bs_refresh_finish,	METH_VARARGS},
    {"bs_head_insert",	(PyCFunction)BlockStore_bs_head_insert,	METH_VARARGS},
    {"bs_head_follow",	(PyCFunction)BlockStore_bs_head_follow,	METH_VARARGS},
    {"bs_head_furthest",(PyCFunction)BlockStore_bs_head_furthest,METH_VARARGS},
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
BlockStoreModule_create(PyObject *self, PyObject *i_args)
{
    char * factname;
    char * instname;
    unsigned long size;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "sskO!:create",
                          &factname, &instname, &size, &PyTuple_Type, &tup))
		return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "BlockStore::create arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkBlockStoreObject(BlockStoreFactory::create(factname,
                                                            instname,
                                                            size,
                                                            args));
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStoreModule_open(PyObject *self, PyObject *i_args)
{
    char * factname;
    char * instname;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "ssO!:open",
                          &factname, &instname, &PyTuple_Type, &tup))
		return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "BlockStore::open arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkBlockStoreObject(BlockStoreFactory::open(factname,
                                                          instname,
                                                          args));
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStoreModule_lookup(PyObject *self, PyObject *i_args)
{
    char * instname;
    if (!PyArg_ParseTuple(i_args, "s:lookup", &instname))
		return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        return mkBlockStoreObject(BlockStoreFactory::lookup(instname));
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStoreModule_unmap(PyObject *self, PyObject *i_args)
{
    char * instname;
    if (!PyArg_ParseTuple(i_args, "s:unmap", &instname))
		return NULL;

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        BlockStoreFactory::unmap(instname);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyObject *
BlockStoreModule_destroy(PyObject *self, PyObject *i_args)
{
    char * name;
    PyObject * tup;
    if (!PyArg_ParseTuple(i_args, "sO!:destroy", &name, &PyTuple_Type, &tup))
		return NULL;

    StringSeq args;
    for (int i = 0; i < PyTuple_Size(tup); ++i)
    {
        PyObject * str = PyTuple_GetItem(tup, i);
        if (!PyString_Check(str))
        {
            PyErr_SetString(PyExc_TypeError,
                            "BlockStore::destroy arg must be tuple of strings");
            return NULL;
        }

        args.push_back(std::string(PyString_AsString(str)));
    }

    PYUTP_TRY
    {
        PYUTP_THREADED_SCOPE scope;
        BlockStoreFactory::destroy(name, args);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PYUTP_CATCH_ALL;
}

static PyMethodDef BlockStoreModule_methods[] = {
	{"create",			BlockStoreModule_create,			METH_VARARGS},
    {"open",			BlockStoreModule_open,				METH_VARARGS},
    {"lookup",			BlockStoreModule_lookup,			METH_VARARGS},
    {"unmap",			BlockStoreModule_unmap,				METH_VARARGS},
    {"destroy",			BlockStoreModule_destroy,			METH_VARARGS},
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
        PyErr_NewException((char *) "BlockStore.error", NULL, NULL);
	PyDict_SetItemString(d, "error", BlockStoreErrorObject);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
