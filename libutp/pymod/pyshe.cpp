#include <string>

#if defined(LINUX)
// Python and sys/features clash over this.
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
#endif

#include <Python.h>
#include <structseq.h>

#include "HeadEdge.pb.h"

#include "pyshe.h"

using namespace std;

namespace utp {

PyTypeObject SignedHeadEdgeType;

PyDoc_STRVAR(she_result__doc__,
"she_result: Signed Head Node.\n\n\
This object may be accessed either as a tuple\n\
 or via attributes.");

static PyStructSequence_Field she_result_fields[] =
{
    {(char *) "fstag",		(char *) "digest of file system id"},
    {(char *) "rootref",	(char *) "encrypted root blockref"},
    {(char *) "prevref",	(char *) "encrypted prev root blockref"},
    {(char *) "tstamp",		(char *) "modification timestamp"},
    {(char *) "keyid",		(char *) "key identifier"},
    {(char *) "signature",	(char *) "digital signature"},
    {0}
};

static PyStructSequence_Desc she_result_desc = {
	(char *) "utp.SignedHeadEdge", /* name */
	she_result__doc__, /* doc */
	she_result_fields,
    6
};

static newfunc structseq_new;

static PyObject *
sheresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyStructSequence * result;
	result = (PyStructSequence*)structseq_new(type, args, kwds);
	if (!result)
		return NULL;
	return (PyObject*) result;
}

static PyObject *
bytes2buffer(string const & i_str)
{
    // Create a new buffer object.
    PyObject * retobj = PyBuffer_New(i_str.size());

    // Obtain pointers to the internal buffer.
    void * outptr;
    Py_ssize_t outlen;
    if (PyObject_AsWriteBuffer(retobj, &outptr, &outlen))
        return NULL;

    // Copy the data into the buffer.
    memcpy(outptr, i_str.data(), outlen);

    return retobj;
}

/* pack a system stat C structure into the Python stat tuple
   (used by posix_stat() and posix_fstat()) */
PyObject *
pyshe_fromprotoshe(SignedHeadEdge const & i_she)
{
	PyObject * v = PyStructSequence_New(&SignedHeadEdgeType);
	if (v == NULL)
		return NULL;

    // Unmarshal the signed head node.
    HeadEdge he;
    he.ParseFromString(i_she.headedge());

    unsigned ndx = 0;

    PyErr_Clear();
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(he.fstag()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(he.rootref()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(he.prevref()));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) he.tstamp()));

    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(i_she.keyid()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(i_she.signature()));

	if (PyErr_Occurred()) {
		Py_DECREF(v);
		return NULL;
	}

	return v;
}

static string
buffer2bytes(PyObject * bufobj)
{
    void const * ptr;
    Py_ssize_t len;
    if (PyObject_AsReadBuffer(bufobj, &ptr, &len))
    {
        PyErr_Clear();
        return string();
    }

    return string((char const *) ptr, len);
}

void pyshe_asprotoshe(PyObject * sheobj, SignedHeadEdge & sheref)
{
    HeadEdge he;

    int ndx = 0;

    // First fields go into a HeadEdge.
    he.set_fstag(buffer2bytes(PySequence_GetItem(sheobj, ndx++)));
    he.set_rootref(buffer2bytes(PySequence_GetItem(sheobj, ndx++)));
    he.set_prevref(buffer2bytes(PySequence_GetItem(sheobj, ndx++)));
    he.set_tstamp(PyLong_AsLongLong(PySequence_GetItem(sheobj, ndx++)));

    // Serialize the HeadEdge into the SignedHeadEdge.
    he.SerializeToString(sheref.mutable_headedge());

    // FIXME - We should be signing here!

    // Set the keyid and signature.
    sheref.set_keyid(buffer2bytes(PySequence_GetItem(sheobj, ndx++)));
    sheref.set_signature(buffer2bytes(PySequence_GetItem(sheobj, ndx++)));
}

#if defined(WIN32)
__declspec( dllexport )
#endif
void
init_she(PyObject * m)
{
    she_result_desc.name = (char *) "utp" ".SignedHeadEdge";
    PyStructSequence_InitType(&SignedHeadEdgeType, &she_result_desc);
    structseq_new = SignedHeadEdgeType.tp_new;
    SignedHeadEdgeType.tp_new = sheresult_new;

	Py_INCREF((PyObject*) &SignedHeadEdgeType);
	PyModule_AddObject(m, "SignedHeadEdge", (PyObject*) &SignedHeadEdgeType);
}

} // end namespace utp
