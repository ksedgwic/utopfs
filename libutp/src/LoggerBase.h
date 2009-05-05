#ifndef utp_LoggerBase_h__
#define utp_LoggerBase_h__

/// @file LoggerBase.h
/// Defines Logger Interface Base Implementation.

#include "Log.h"
#include "utpexp.h"

namespace utp {

/// Implementation Base for the Logger Interface.
class UTP_EXP LoggerBase : public Logger
{
public:
    /// Constructor.
    LoggerBase(int i_level, LogFormatter * i_formatter);

    /// Destructor.
    virtual ~LoggerBase();
        
    virtual bool is_enabled(int i_level) const;

    virtual int level() const;

    virtual void level(int i_level);

    virtual void formatter(LogFormatter * i_formatter);

    virtual LogFormatter * formatter() const;

protected:
    int					m_level;
    LogFormatter *		m_formatter;
};

} // end namespace utp


// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_LoggerBase_h__
