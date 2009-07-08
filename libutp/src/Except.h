#ifndef utp_Except_h__
#define utp_Except_h__

/// @file Except.h
/// Exceptions.

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include "utpexp.h"

namespace utp {

class UTP_EXC_EXP Exception : public std::exception
{
public:
    enum Type
    {
        T_BASE,
        T_INTERNAL,
        T_OPERATION,
        T_NOTFOUND,
        T_NOTUNIQUE,
        T_VALUE,
        T_PARSE,
        T_VERIFICATION,
    };

    virtual const char * what() const throw();

    virtual ~Exception() throw() {}

    virtual const char * details() const throw() { return m_details.c_str(); }

    virtual Type type() const { return T_BASE; }

protected:
    Exception(std::string const & i_base,
              std::string const & i_details);

    mutable std::string		m_buffer;
    mutable std::string		m_details;

#if defined(LINUX)
    // Linux embedds a stack trace for debugging.
    enum { NFRAMES = 16 };
    void *					m_btdata[NFRAMES];
    size_t					m_btsize;
#endif
};

class UTP_EXC_EXP InternalError : public Exception
{
public:
    InternalError(std::string const & i_details)
        : Exception("InternalError", i_details) {}

    virtual Type type() const { return T_INTERNAL; }
};

class UTP_EXC_EXP OperationError : public Exception
{
public:
    OperationError(std::string const & i_details)
        : Exception("OperationError", i_details) {}

    virtual Type type() const { return T_OPERATION; }
};

class UTP_EXC_EXP NotFoundError : public Exception
{
public:
    NotFoundError(std::string const & i_details)
        : Exception("NotFoundError", i_details) {}

    virtual Type type() const { return T_NOTFOUND; }
};

class UTP_EXC_EXP NotUniqueError : public Exception
{
public:
    NotUniqueError(std::string const & i_details)
        : Exception("NotUniqueError", i_details) {}

    virtual Type type() const { return T_NOTUNIQUE; }
};

class UTP_EXC_EXP ValueError : public Exception
{
public:
    ValueError(std::string const & i_details)
        : Exception("ValueError", i_details) {}

    virtual Type type() const { return T_VALUE; }
};

class UTP_EXC_EXP ParseError : public Exception
{
public:
    ParseError(std::string const & i_details)
        : Exception("ParseError", i_details) {}

    virtual Type type() const { return T_PARSE; }
};

class UTP_EXC_EXP VerificationError : public Exception
{
public:
    VerificationError(std::string const & i_details)
        : Exception("VerificationError", i_details) {}

    virtual Type type() const { return T_VERIFICATION; }
};

// The throwstream macro assembles the string argument to the
// exception constructor from an iostream.
//
#define throwstream(__except, __msg)                \
    do {                                            \
        std::ostringstream __ostrm;                 \
        __ostrm << __msg;                           \
        throw __except(__ostrm.str().c_str());      \
    } while (false)

#define FILELINE	__FILE__ << ':' << __LINE__ << ' '

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Except_h__
