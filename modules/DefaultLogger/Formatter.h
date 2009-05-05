#ifndef DFLG_Formatter_h__
#define DFLG_Formatter_h__

/// @file Formatter.h
/// Defines the Default Logging Formatter Implementation

#include <string>
#include <iostream>

#include "Log.h"
#include "FormatterBase.h"

#include "dflgexp.h"

namespace DFLG {

/// Default Logging Formatter
class DFLG_EXP Formatter : public utp::FormatterBase
{
public:
    /// Returns the singleton instance or NULL.
    static LogFormatter * getInstance();

    /// Default constructor.
    Formatter() {}

    /// Destructor.
    virtual ~Formatter() {}

    /// Formats a ValueSeq for the log.
    /// @param valseq the values to format.
    virtual std::string
    format(utp::LogFieldSeq const & i_valseq) const;
};

} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLG_Formatter_h__
