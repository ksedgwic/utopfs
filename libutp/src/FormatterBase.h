#ifndef utp_FormatterBase_h__
#define utp_FormatterBase_h__

/// @file FormatterBase.h
/// Defines Formatter Implementation Base

#include <string>
#include <iostream>

#include "Log.h"
#include "utpexp.h"

namespace utp {

/// Formatter Implementation Base
class UTP_EXP FormatterBase : public LogFormatter
{
public:
    /// Default constructor.
    FormatterBase();

    /// Destructor.
    virtual ~FormatterBase();
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_FormatterBase_h__
