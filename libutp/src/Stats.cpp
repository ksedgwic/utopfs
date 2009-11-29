#include <iostream>
#include <string>

#include "Stats.h"
#include "Stats.pb.h"
#include "utplog.h"

using namespace std;

namespace utp {

StatRec * Stats::set(StatSet & o_ss,
                     string const & i_name,
                     int64_t i_value,
                     string const & i_format,
                     StatFormatType i_type)
{
    StatRec * srp = o_ss.add_rec();
    srp->set_name(i_name);
    srp->set_value(i_value);
    StatFormat * sfp = srp->add_format();
    sfp->set_fmtstr(i_format);
    sfp->set_fmttype(i_type);

    return srp;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
