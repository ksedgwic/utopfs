#ifndef utp_Log_h__
#define utp_Log_h__

/// @file Log.h

#include <iosfwd>
#include <sstream>
#include <vector>
#include <string>

#include "utpexp.h"
#include "LogField.h"

namespace utp {

/// A sequence of LogFieldHandle objects.
typedef std::vector<LogFieldHandle> LogFieldSeq;

/// Abstract interface for formatting objects.
class UTP_EXP LogFormatter
{
public:
    virtual ~LogFormatter() {}

    /// Converts a LogFieldSeq into a formatted logging entry.
    /// @param valseq the list of values to be formatted.
    virtual std::string format(LogFieldSeq const & valseq) const = 0;
};

/// Abstract interface for logging objects.
class UTP_EXP Logger
{
public:
    virtual ~Logger() {}

    /// Tests whether this Logger is enabled at a given level.
    /// @param level logging level, 0 = none, 9 = lots.
    virtual bool is_enabled(int level) const = 0;

    /// Logs the LogFieldSeq if the level is appropriate.
    /// @param level the desired logging level.
    /// @param valseq a ValSeq list of LogLogField objects.
    virtual void debug_str(int level, LogFieldSeq const & valseq) = 0;
};

extern class LogCategory UTP_EXP theRootLogCategory;

/// A sequence of Category objects.
typedef std::vector<LogCategory *> LogCategorySeq;

/// A sequence of Logger objects.
typedef std::vector<Logger *> LoggerSeq;

/// A Logger which to be used for a category of related logging points.
class UTP_EXP LogCategory : public Logger
{
public:
    /// Constructor.
    /// @param name the name of this category.
    /// @param parent pointer to this categories parent.
    LogCategory(std::string const & name,
                LogCategory * parent = &theRootLogCategory);

    /// Destructor.
    virtual ~LogCategory();

    virtual bool is_enabled(int level) const;

    virtual void debug_str(int level, LogFieldSeq const & valseq);

    /// Returns the current level of this category.
    int level() const { return m_level; }

    /// Sets the current level of this category.
    /// @param level the new level.
    /// @param recursive should we recursively set subcategories level too?
    void level(int level, bool recursive = true);

    /// Adds a logger to this category.
    /// @param logger the logger being added.
    /// @param recursive should we add this logger to subcategories?
    void logger_add(Logger * logger, bool recursive = true);

    /// Removes a logger from this category.
    /// @param logger the logger being removed.
    /// @param recursive should we remove this logger from subcategories?
    void logger_rem(Logger * logger, bool recursive = true);

    /// Returns the parent category of this category.
    LogCategory * parent() const;

    /// Returns the subcategories of this category.
    void subcategories(LogCategorySeq & outseq) const;

    /// Returns a reference to this categories name.
    std::string const & name() const { return m_name; }

    // INTERNAL USE ONLY ...
    void subcategory_add(LogCategory * subcat);
    void subcategory_rem(LogCategory * subcat);

protected:
    std::string						m_name;
    LogCategory *					m_parent;
    int								m_level;
    mutable ACE_RW_Thread_Mutex		m_logcat_mutex;
    LogCategorySeq					m_subcat;
    LoggerSeq						m_loggers;
};

} // end namespace utp

/// Log point macro
/// @param __logger the LogCategory to use for this point.
/// @param __level the logging level of this point.
/// @param __expr a stream expression to be logged.
#define LOG(__logger, __level, __expr)                              \
    do {                                                            \
        if (__logger.is_enabled(__level)) {                         \
            utp::LogFieldSeq __valseq;                              \
            __valseq.push_back                                      \
                (new utp::FileLineLogField("fileline",              \
                                           __FILE__, __LINE__));    \
            std::ostringstream __buffer;                            \
            __buffer << __expr;                                     \
            __valseq.push_back                                      \
                (new utp::StringLogField("data",                    \
                                         __buffer.str()));          \
            __logger.debug_str(__level, __valseq);                  \
        }                                                           \
    } while (false)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Log_h__
