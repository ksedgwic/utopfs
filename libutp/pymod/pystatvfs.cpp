#include <Python.h>
#include <structseq.h>

#include <sys/statvfs.h>

#include "pystatvfs.h"

namespace utp {

PyTypeObject StatVFSResultType;

PyDoc_STRVAR(statvfs_result__doc__,
"statvfs_result: Result from FileSystem::fs_statfs.\n\n\
This object may be accessed either as a tuple\n\
 or via attributes.");

static PyStructSequence_Field statvfs_result_fields[] =
{
    {(char *) "f_bsize",	(char *) "file system block size"},
    {(char *) "f_frsize",	(char *) "fragment size"},
    {(char *) "f_blocks",	(char *) "size of fs in f_frsize units"},
    {(char *) "f_bfree",	(char *) "# free blocks"},
    {(char *) "f_bavail",	(char *) "# free blocks for non-root"},
    {(char *) "f_files",	(char *) "# inodes"},
    {(char *) "f_ffree",	(char *) "# free inodes"},
    {(char *) "f_favail",	(char *) "# free inodes for non-root"},
    {(char *) "f_fsid",		(char *) "file system ID"},
    {(char *) "f_flag",		(char *) "mount flags"},
    {(char *) "f_namemax",	(char *) "maximum filename length"},
    {0}
};

static PyStructSequence_Desc statvfs_result_desc = {
	(char *) "statvfs_result", /* name */
	statvfs_result__doc__, /* doc */
	statvfs_result_fields,
	11
};

static newfunc structseq_new;

static PyObject *
statvfsresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
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
pystatvfs_fromstructstatvfs(struct statvfs const * stvp)
{
	PyObject *v = PyStructSequence_New(&StatVFSResultType);
	if (v == NULL)
		return NULL;

    unsigned ndx = 0;

    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_bsize));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_frsize));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_blocks));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_bfree));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_bavail));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_files));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_ffree));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_favail));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_fsid));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_flag));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) stvp->f_namemax));

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
init_statvfs(void)
{
    statvfs_result_desc.name = (char *) "utp" ".statvfs_result";
    PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);
    structseq_new = StatVFSResultType.tp_new;
    StatVFSResultType.tp_new = statvfsresult_new;
}

} // end namespace utp
