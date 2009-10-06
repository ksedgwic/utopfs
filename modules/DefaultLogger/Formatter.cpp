#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "LogField.h"

#include "Formatter.h"

using namespace std;
using namespace utp;

namespace DFLG {

LogFormatter *
Formatter::getInstance()
{
    static Formatter * theFormatter = NULL;

    if (!theFormatter)
        theFormatter = new Formatter();

    return theFormatter;
}

string
Formatter::format(LogFieldSeq const & i_valseq) const
{
    ostringstream buffer;

    bool firsttime = true;

    LogFieldSeq::const_iterator pos;
    for (pos = i_valseq.begin(); pos != i_valseq.end(); pos++)
    {
        if (!firsttime)
            buffer << ' ';
        firsttime = false;

        if ((*pos)->getName() == "category")
        {
            // pad the categories with spaces
            string bufstr;
            (*pos)->outstr(bufstr);
            int width = 5;
            int len = bufstr.length();
            buffer << bufstr;
            if (len < width)
                buffer << string(width - len, ' ');
        }
        else if ((*pos)->getName() == "threadid")
        {
            string bufstr;
            (*pos)->outstr(bufstr);
            buffer << 'T' << bufstr;
        }
        else
        {
            string bufstr;
            (*pos)->outstr(bufstr);
            buffer << bufstr;
        }
    }

    return buffer.str();
}

} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

