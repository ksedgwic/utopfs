#ifndef utp_Stats_h__
#define utp_Stats_h__

/// @file Stats.h
/// Utopia FileSystem Stats Object.
///
/// Stats Object for Utopfs FileSystem.

#include <iosfwd>

#include "Stats.pb.h"

#include "utpfwd.h"
#include "utpexp.h"

namespace utp {

class UTP_EXP Stats
{
public:
    static StatRec * set(StatSet & o_ss,
                         std::string const & i_name,
                         int64_t i_value,
                         std::string const & i_format,
                         StatFormatType i_type);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Stats_h__
