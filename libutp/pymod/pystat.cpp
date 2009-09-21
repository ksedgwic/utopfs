// This code is based on posixmodule.c from th Python source.

#include <Python.h>
#include <structseq.h>

#include "pystat.h"

namespace utp {

PyTypeObject StatResultType;

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
#define ST_BLKSIZE_IDX 13
#else
#define ST_BLKSIZE_IDX 12
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
#define ST_BLOCKS_IDX (ST_BLKSIZE_IDX+1)
#else
#define ST_BLOCKS_IDX ST_BLKSIZE_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
#define ST_RDEV_IDX (ST_BLOCKS_IDX+1)
#else
#define ST_RDEV_IDX ST_BLOCKS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_GEN
#define ST_GEN_IDX (ST_FLAGS_IDX+1)
#else
#define ST_GEN_IDX ST_FLAGS_IDX
#endif

PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,\n\
or st_flags, they are available as attributes only.\n\
\n\
See os.stat for more information.");

static PyStructSequence_Field stat_result_fields[] = {
	{(char *) "st_mode",    (char *) "protection bits"},
	{(char *) "st_ino",     (char *) "inode"},
	{(char *) "st_dev",     (char *) "device"},
	{(char *) "st_nlink",   (char *) "number of hard links"},
	{(char *) "st_uid",     (char *) "user ID of owner"},
	{(char *) "st_gid",     (char *) "group ID of owner"},
	{(char *) "st_size",    (char *) "total size, in bytes"},
	/* The NULL is replaced with PyStructSequence_UnnamedField later. */
	{(char *) NULL,   (char *) "integer time of last access"},
	{(char *) NULL,   (char *) "integer time of last modification"},
	{(char *) NULL,   (char *) "integer time of last change"},
	{(char *) "st_atime",   (char *) "time of last access"},
	{(char *) "st_mtime",   (char *) "time of last modification"},
	{(char *) "st_ctime",   (char *) "time of last change"},
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	{(char *) "st_blksize", (char *) "blocksize for filesystem I/O"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	{(char *) "st_blocks",  (char *) "number of blocks allocated"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	{(char *) "st_rdev",    (char *) "device type (if inode device)"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
	{(char *) "st_flags",   (char *) "user defined flags for file"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
	{(char *) "st_gen",    (char *) "generation number"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
	{(char *) "st_birthtime",   (char *) "time of creation"},
#endif
	{0}
};

#ifdef WIN32
char *PyStructSequence_UnnamedField = "unnamed field";  
#endif

static PyStructSequence_Desc stat_result_desc = {
	(char *) "stat_result", /* name */
	stat_result__doc__, /* doc */
	stat_result_fields,
	10
};

static newfunc structseq_new;

static PyObject *
statresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyStructSequence *result;
	int i;

	result = (PyStructSequence*)structseq_new(type, args, kwds);
	if (!result)
		return NULL;
	/* If we have been initialized from a tuple,
	   st_?time might be set to None. Initialize it
	   from the int slots.  */
	for (i = 7; i <= 9; i++) {
		if (result->ob_item[i+3] == Py_None) {
			Py_DECREF(Py_None);
			Py_INCREF(result->ob_item[i]);
			result->ob_item[i+3] = result->ob_item[i];
		}
	}
	return (PyObject*)result;
}

/* If true, st_?time is float. */
static int _stat_float_times = 1;

static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
	PyObject *fval,*ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
	ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
	ival = PyInt_FromLong((long)sec);
#endif
	if (!ival)
		return;
	if (_stat_float_times) {
		fval = PyFloat_FromDouble(sec + 1e-9*nsec);
	} else {
		fval = ival;
		Py_INCREF(fval);
	}
	PyStructSequence_SET_ITEM(v, index, ival);
	PyStructSequence_SET_ITEM(v, index+3, fval);
}

/* pack a system stat C structure into the Python stat tuple
   (used by posix_stat() and posix_fstat()) */
PyObject *
pystat_fromstructstat(struct statstb const * st)
{
	unsigned long ansec, mnsec, cnsec;
	PyObject *v = PyStructSequence_New(&StatResultType);
	if (v == NULL)
		return NULL;

    PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 1,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
    PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG) && !defined(MS_WINDOWS)
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
    PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long)st->st_dev));
#endif
    PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long)st->st_nlink));
    PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long)st->st_uid));
    PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 6,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
    PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong(st->st_size));
#endif

#if defined(HAVE_STAT_TV_NSEC)
	ansec = st->st_atim.tv_nsec;
	mnsec = st->st_mtim.tv_nsec;
	cnsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STAT_TV_NSEC2)
	ansec = st->st_atimespec.tv_nsec;
	mnsec = st->st_mtimespec.tv_nsec;
	cnsec = st->st_ctimespec.tv_nsec;
#elif defined(HAVE_STAT_NSEC)
	ansec = st->st_atime_nsec;
	mnsec = st->st_mtime_nsec;
	cnsec = st->st_ctime_nsec;
#else
	ansec = mnsec = cnsec = 0;
#endif
	fill_time(v, 7, st->st_atime, ansec);
	fill_time(v, 8, st->st_mtime, mnsec);
	fill_time(v, 9, st->st_ctime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
			 PyInt_FromLong((long)st->st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
			 PyInt_FromLong((long)st->st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
			 PyInt_FromLong((long)st->st_rdev));
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
	PyStructSequence_SET_ITEM(v, ST_GEN_IDX,
			 PyInt_FromLong((long)st->st_gen));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
	{
	  PyObject *val;
	  unsigned long bsec,bnsec;
	  bsec = (long)st->st_birthtime;
#ifdef HAVE_STAT_TV_NSEC2
	  bnsec = st->st_birthtimespec.tv_nsec;
#else
	  bnsec = 0;
#endif
	  if (_stat_float_times) {
	    val = PyFloat_FromDouble(bsec + 1e-9*bnsec);
	  } else {
	    val = PyInt_FromLong((long)bsec);
	  }
	  PyStructSequence_SET_ITEM(v, ST_BIRTHTIME_IDX,
				    val);
	}
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
	PyStructSequence_SET_ITEM(v, ST_FLAGS_IDX,
			 PyInt_FromLong((long)st->st_flags));
#endif

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
init_stat(void)
{
    stat_result_desc.name = (char *) "utp" ".stat_result";
    stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
    stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
    stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
    PyStructSequence_InitType(&StatResultType, &stat_result_desc);
    structseq_new = StatResultType.tp_new;
    StatResultType.tp_new = statresult_new;
}

} // end namespace utp
