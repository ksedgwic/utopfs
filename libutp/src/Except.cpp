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

// ---- InternalError

InternalError::InternalError(string const & i_details)
    : Exception("InternalError", i_details)
{}

InternalError::InternalError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
InternalError::type() const
{
    return T_INTERNAL;
}

Exception *
InternalError::clone() const
{
    return new InternalError(*this);
}

// ---- OperationError

OperationError::OperationError(string const & i_details)
    : Exception("OperationError", i_details)
{}

OperationError::OperationError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
OperationError::type() const
{
    return T_OPERATION;
}

Exception *
OperationError::clone() const
{
    return new OperationError(*this);
}

// ---- NotFoundError

NotFoundError::NotFoundError(string const & i_details)
    : Exception("NotFoundError", i_details)
{}

NotFoundError::NotFoundError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
NotFoundError::type() const
{
    return T_NOTFOUND;
}

Exception *
NotFoundError::clone() const
{
    return new NotFoundError(*this);
}

// ---- NotUniqueError

NotUniqueError::NotUniqueError(string const & i_details)
    : Exception("NotUniqueError", i_details)
{}

NotUniqueError::NotUniqueError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
NotUniqueError::type() const
{
    return T_NOTUNIQUE;
}

Exception *
NotUniqueError::clone() const
{
    return new NotUniqueError(*this);
}

// ---- ValueError

ValueError::ValueError(string const & i_details)
    : Exception("ValueError", i_details)
{}

ValueError::ValueError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
ValueError::type() const
{
    return T_VALUE;
}

Exception *
ValueError::clone() const
{
    return new ValueError(*this);
}

// ---- ParseError

ParseError::ParseError(string const & i_details)
    : Exception("ParseError", i_details)
{}

ParseError::ParseError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
ParseError::type() const
{
    return T_PARSE;
}

Exception *
ParseError::clone() const
{
    return new ParseError(*this);
}

// ---- VerificationError

VerificationError::VerificationError(string const & i_details)
    : Exception("VerificationError", i_details)
{}

VerificationError::VerificationError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
VerificationError::type() const
{
    return T_VERIFICATION;
}

Exception *
VerificationError::clone() const
{
    return new VerificationError(*this);
}

// ---- NoSpaceError

NoSpaceError::NoSpaceError(string const & i_details)
    : Exception("NoSpaceError", i_details)
{}

NoSpaceError::NoSpaceError(Exception const & i_ex)
    : Exception(i_ex)
{}

Exception::Type
NoSpaceError::type() const
{
    return T_NOSPACE;
}

Exception *
NoSpaceError::clone() const
{
    return new NoSpaceError(*this);
}

} // end namespace utp


// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
