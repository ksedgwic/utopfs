#include <stdio.h>

#if defined(WIN32)
#define snprintf _snprintf
#endif

#include <sstream>
#include <string>

#include "T64.h"
#include "LogField.h"

using namespace std;

namespace utp {

void
StringLogField::outstr(string & o_str) const
{
    o_str = m_value;
}

void
TimeLogField::outstr(string & o_str) const
{
    ostringstream buf;
    buf << m_timeval;
    o_str = buf.str();
}

void
IntegerLogField::outstr(string & o_str) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d", m_value);
    o_str = string(buffer);
}

void
FileLineLogField::outstr(string & o_str) const
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s:%d", m_file.c_str(), m_line);
    o_str = string(buffer);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
