#include <Python.h>
#include <structseq.h>

#include "pybsstat.h"

namespace utp {

PyTypeObject BlockStoreStatResultType;

PyDoc_STRVAR(bsstat_result__doc__,
"bsstat_result: Result from BlockStore::bs_stat.\n\n\
This object may be accessed either as a tuple of\n\
 (size, free) or via the attributes bss_size and\n\
 bss_free.");

static PyStructSequence_Field bsstat_result_fields[] =
{
	{(char *) "bss_size",	(char *) "total data size in bytes"},
	{(char *) "bss_free",	(char *) "uncommitted size in bytes"},
    {0}
};

static PyStructSequence_Desc bsstat_result_desc = {
	(char *) "bsstat_result", /* name */
	bsstat_result__doc__, /* doc */
	bsstat_result_fields,
	2
};

static newfunc structseq_new;

static PyObject *
bsstatresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyStructSequence * result;
	result = (PyStructSequence*)structseq_new(type, args, kwds);
	if (!result)
		return NULL;
	return (PyObject*) result;
}

/* pack a system stat C structure into the Python stat tuple
   (used by posix_stat() and posix_fstat()) */
PyObject *
pybsstat_fromstructbsstat(struct BlockStore::Stat const * bsst)
{
	PyObject *v = PyStructSequence_New(&BlockStoreStatResultType);
	if (v == NULL)
		return NULL;

    PyStructSequence_SET_ITEM(v, 0, PyLong_FromLongLong
                              ((PY_LONG_LONG) bsst->bss_size));
    PyStructSequence_SET_ITEM(v, 1, PyLong_FromLongLong
                              ((PY_LONG_LONG) bsst->bss_free));

	if (PyErr_Occurred()) {
		Py_DECREF(v);
		return NULL;
	}

	return v;
}

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_bsstat(void)
{
    bsstat_result_desc.name = (char *) "utp" ".bsstat_result";
    PyStructSequence_InitType(&BlockStoreStatResultType, &bsstat_result_desc);
    structseq_new = BlockStoreStatResultType.tp_new;
    BlockStoreStatResultType.tp_new = bsstatresult_new;
}

} // end namespace utp
