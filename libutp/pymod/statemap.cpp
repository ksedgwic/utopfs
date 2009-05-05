#include "statemap.h"

using namespace std;

namespace utp {

// instantiate the static "global"
ACE_TSS<PyBindingMap> StateMap::g_mapp;

void
StateMap::initialize()
{
    // don't need this with ACE .. but keep around for others ...
}

PyThreadState *
StateMap::associate(PyInterpreterState * interp)
{
    PyThreadState * tstate;
    PyBindingMap::iterator i = g_mapp->find(interp);
    if (i == g_mapp->end()) {
        // doesn't exist yet ...
        tstate = PyThreadState_New(interp);
        g_mapp->insert(make_pair(interp, tstate));
    } else {
        // found it
        tstate = i->second;
    }

    return tstate;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// End:

