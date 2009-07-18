#ifndef utp_Except_h__
#define utp_Except_h__

/// @file Except.h
/// Exceptions.

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include <ace/OS_NS_string.h>

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
        T_NOSPACE,
    };

    Exception(Exception const & i_ex);

    virtual ~Exception() throw();

    virtual const char * what() const throw();

    virtual const char * details() const throw();

    virtual Type type() const;

    virtual Exception * clone() const;

    virtual void rethrow() const;

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
    InternalError(std::string const & i_details);
    InternalError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP OperationError : public Exception
{
public:
    OperationError(std::string const & i_details);
    OperationError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP NotFoundError : public Exception
{
public:
    NotFoundError(std::string const & i_details);
    NotFoundError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP NotUniqueError : public Exception
{
public:
    NotUniqueError(std::string const & i_details);
    NotUniqueError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP ValueError : public Exception
{
public:
    ValueError(std::string const & i_details);
    ValueError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP ParseError : public Exception
{
public:
    ParseError(std::string const & i_details);
    ParseError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP VerificationError : public Exception
{
public:
    VerificationError(std::string const & i_details);
    VerificationError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
};

class UTP_EXC_EXP NoSpaceError : public Exception
{
public:
    NoSpaceError(std::string const & i_details);
    NoSpaceError(Exception const & i_ex);
    virtual Type type() const;
    virtual Exception * clone() const;
    virtual void rethrow() const;
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
