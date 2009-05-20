#include "Base32.h"

using namespace std;
using namespace utp;

namespace {

// NOTE - RFC 3548
// http://tools.ietf.org/html/rfc3548

const char base32map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

#define MAP(x) (base32map[x])
#define UNMAP(c) ((c < 'A') ? 26 + (c - '2') : (c - 'A'))

uint8 const *
chunkEncode(uint8 const * in,
            uint8 const * end,
            string & out)
{
    uint8 inbuf[5] = { 0, 0, 0, 0, 0 };
    int sz = 0;
    while (sz < 5) {
        inbuf[sz++] = *in++;
        if (in == end) break;
    }

    int count = 0;

    if (sz > 0) {
        out += MAP(((inbuf[0] & 0xf8) >> 3));
        out += MAP(((inbuf[0] & 0x07) << 2) |
                   ((inbuf[1] & 0xc0) >> 6));
        count += 2;
    }

    if (sz > 1) {
        out += MAP(((inbuf[1] & 0x3e) >> 1));
        out += MAP(((inbuf[1] & 0x01) << 4) |
                   ((inbuf[2] & 0xf0) >> 4));
        count += 2;
    }

    if (sz > 2) {
        out += MAP(((inbuf[2] & 0x0f) << 1) |
                   ((inbuf[3] & 0x80) >> 7));
        count += 1;
    }

    if (sz > 3) {
        out += MAP(((inbuf[3] & 0x7c) >> 2));
        out += MAP(((inbuf[3] & 0x03) << 3) |
                   ((inbuf[4] & 0xe0) >> 5));
        count += 2;
    }

    if (sz > 4) {
        out += MAP(((inbuf[4] & 0x1f)));
        count += 1;
    }

    // pad any remaining postions
    for (int i = count; i < 8; i++)
        out += '=';

    return in;
}

} // end namespace

namespace utp {

string const
Base32::encode(void const * i_data, size_t i_size)
{
    string retval;
    encode((uint8 const *) i_data, i_size, retval);
    return retval;
}

void
Base32::encode(uint8 const * i_raw,
               size_t const & i_size,
               string & o_encoded)
{
    o_encoded.erase();
    o_encoded.reserve((((i_size - 1)/5) + 1) * 8);

    uint8 const * pos = i_raw;
    uint8 const * end = i_raw + i_size;

    while (pos != end)
        pos = chunkEncode(pos, end, o_encoded);
}

string const
Base32::encode(OctetSeq const & i_data)
{
    string retstr;
    encode(&i_data[0], i_data.size(), retstr);
    return retstr;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
