#ifndef utp_LogField_h__
#define utp_LogField_h__

/// @file LogField.h
/// Defines Logging value types.

#include <iostream>
#include <string>

#include "T64.h"
#include "RC.h"

#include "utpexp.h"

namespace utp {

/// Abstract Logging field name, value interface.
class UTP_EXP LogField : public virtual RCObj
{
public:
    /// Returns a reference to the name of the logging field.
    virtual std::string const & getName() const = 0;
        
    /// Marshals the value into a string appropriate for logging.
    virtual void outstr(std::string & o_string) const = 0;
};

/// A reference counted handle to LogField objects.
typedef RCPtr<LogField> LogFieldHandle;

/// Implementation base for LogField implementations.
class UTP_EXP LogFieldBase : public LogField
{
public:
    /// Constructor.
    /// @param name the name of the logging field.
    LogFieldBase(std::string const & name) : m_name(name) {}
        
    std::string const & getName() const { return m_name; }
protected:
    std::string		m_name;
};

/// LogField which holds a std::string.
class UTP_EXP StringLogField : public LogFieldBase
{
public:
    /// Constructor.
    /// @param name the name of the logging field.
    /// @param value the string to be logged.
    StringLogField(std::string const & name, std::string const & value)
        : LogFieldBase(name)
        , m_value(value)
    {}
    void outstr(std::string & o_string) const;
protected:
    std::string		m_value;
};

/// LogField which holds a T64.
class UTP_EXP TimeLogField : public LogFieldBase
{
public:
    /// Constructor.
    /// @param name the name of the logging field.
    /// @param timeval the T64 to be logged.
    TimeLogField(std::string const & name, T64 const & timeval)
        : LogFieldBase(name)
        , m_timeval(timeval)
    {}
    void outstr(std::string & o_string) const;
protected:
    T64	m_timeval;
};

/// LogField which holds an integer.
class UTP_EXP IntegerLogField : public LogFieldBase
{
public:
    /// Constructor.
    /// @param name the name of the logging field.
    /// @param value the integer value to be logged.
    IntegerLogField(std::string const & name, int value)
        : LogFieldBase(name)
        , m_value(value)
    {}
    void outstr(std::string & o_string) const;
protected:
    int				m_value;
};

/// LogField which holds a file and line location.
class UTP_EXP FileLineLogField : public LogFieldBase
{
public:
    /// Constructor.
    /// @param name the name of the logging field.
    /// @param file the name of the file.
    /// @param line the line number in the file.
    FileLineLogField(std::string const & name,
                     std::string const & file,
                     int line)
        : LogFieldBase(name)
        , m_file(file)
        , m_line(line)
    {}
    void outstr(std::string & o_string) const;
protected:
    std::string		m_file;
    int				m_line;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_LogField_h__
