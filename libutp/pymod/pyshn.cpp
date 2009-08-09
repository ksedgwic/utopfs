#include <string>

#if defined(LINUX)
// Python and sys/features clash over this.
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
#endif

#include <Python.h>
#include <structseq.h>

#include "HeadNode.pb.h"

#include "pyshn.h"

using namespace std;

namespace utp {

PyTypeObject SignedHeadNodeResultType;

PyDoc_STRVAR(shn_result__doc__,
"shn_result: Signed Head Node.\n\n\
This object may be accessed either as a tuple\n\
 or via attributes.");

static PyStructSequence_Field shn_result_fields[] =
{
    {(char *) "fstag",		(char *) "digest of file system id"},
    {(char *) "rootref",	(char *) "encrypted root blockref"},
    {(char *) "prevref",	(char *) "encrypted prev root blockref"},
    {(char *) "tstamp",		(char *) "modification timestamp"},
    {(char *) "keyid",		(char *) "key identifier"},
    {(char *) "signature",	(char *) "digital signature"},
    {0}
};

static PyStructSequence_Desc shn_result_desc = {
	(char *) "utp.SignedHeadNode", /* name */
	shn_result__doc__, /* doc */
	shn_result_fields,
    6
};

static newfunc structseq_new;

static PyObject *
shnresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
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
pyshn_fromprotoshn(SignedHeadNode const & i_shn)
{
	PyObject * v = PyStructSequence_New(&SignedHeadNodeResultType);
	if (v == NULL)
		return NULL;

    // Unmarshal the signed head node.
    HeadNode hn;
    hn.ParseFromString(i_shn.headnode());

    unsigned ndx = 0;

    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(hn.fstag()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(hn.rootref()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(hn.prevref()));
    PyStructSequence_SET_ITEM(v, ndx++, PyLong_FromLongLong
                              ((PY_LONG_LONG) hn.tstamp()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(i_shn.keyid()));
    PyStructSequence_SET_ITEM(v, ndx++, bytes2buffer(i_shn.signature()));

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
init_shn(PyObject * m)
{
    shn_result_desc.name = (char *) "utp" ".SignedHeadNode";
    PyStructSequence_InitType(&SignedHeadNodeResultType, &shn_result_desc);
    structseq_new = SignedHeadNodeResultType.tp_new;
    SignedHeadNodeResultType.tp_new = shnresult_new;

	Py_INCREF((PyObject*) &SignedHeadNodeResultType);
	PyModule_AddObject(m,
                       "SignedHeadNode",
                       (PyObject*) &SignedHeadNodeResultType);
}

} // end namespace utp
