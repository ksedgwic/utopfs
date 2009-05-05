/// @file Except.cpp

#include "Except.h"

#if defined(LINUX)
#include <dlfcn.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <regex.h>
#include <sstream>
#include <stdlib.h>
#endif

using namespace std;

namespace utp {

char const *
Exception::what() const throw()
{
#if defined(LINUX)
    std::ostringstream ostrm;
    ostrm << m_buffer << endl;
    for (unsigned i = 0; i < m_btsize; ++i)
    {
        Dl_info dlinfo;
        if (dladdr(m_btdata[i], & dlinfo) == 0)
        {
            // Couldn't find the symbol information at all.
            ostrm << "  " << m_btdata[i] << endl;
        }
        else
        {
            int status;
            char * demangled = abi::__cxa_demangle(dlinfo.dli_sname,
                                                   NULL,
                                                   NULL,
                                                   &status);
            if (demangled)
            {
                // Successfully demangled.
                ostrm << "  " << demangled << endl;
                free(demangled);
            }
            else if (dlinfo.dli_sname)
            {
                // Couldn't demangle, but we have a symbol name.
                ostrm << "  " << dlinfo.dli_sname << endl;
            }
            else
            {
                ostrm << "  addr2line -C -s -f -e "
                      << dlinfo.dli_fname << ' ' << m_btdata[i]
                      << endl;
            }
        }
    }
    m_buffer = ostrm.str();
#endif
    return m_buffer.c_str();
}

Exception::Exception(string const & i_base, string const & i_details)
    : m_buffer(i_base + ": " + i_details)
    , m_details(i_details)
{
#if defined(LINUX)
    m_btsize = backtrace(m_btdata, NFRAMES);
#endif
}

} // end namespace utp


// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
