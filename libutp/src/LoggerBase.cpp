#include "LoggerBase.h"

namespace utp {

/// Constructor.
LoggerBase::LoggerBase(int i_level, LogFormatter * i_formatter)
    : m_level(i_level)
    , m_formatter(i_formatter)
{
}

/// Destructor.
LoggerBase::~LoggerBase()
{
}
        
bool
LoggerBase::is_enabled(int i_level) const
{
    return m_level >= i_level;
}

int
LoggerBase::level() const
{
    return m_level;
}

void
LoggerBase::level(int i_level)
{
    m_level = i_level;
}

void
LoggerBase::formatter(LogFormatter * i_formatter)
{
    m_formatter = i_formatter;
}

LogFormatter *
LoggerBase::formatter() const
{
    return m_formatter;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

