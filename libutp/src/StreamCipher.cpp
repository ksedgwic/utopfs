#include <ace/OS_NS_string.h>
#include <ace/os_include/os_byteswap.h>

#include "Except.h"
#include "StreamCipher.h"

using namespace std;
using namespace utp;

namespace utp {

StreamCipher::StreamCipher(uint8 const * i_keyp, size_t i_keysz)
{
    uint8 buffer[AES_BLOCK_SIZE];
    ACE_OS::memset(buffer, '\0', sizeof(buffer));
    ACE_OS::memcpy(buffer, i_keyp, min(i_keysz, sizeof(buffer)));
    AES_set_encrypt_key(buffer, 128, &m_key);
}

void
StreamCipher::encrypt(uint8 const * i_ivptr,
                      size_t i_offset,
                      uint8 * io_data,
                      size_t i_size)
{
    unsigned char * in = io_data;
    unsigned char * out = io_data;
	unsigned char ivec[AES_BLOCK_SIZE];
	unsigned char ecount_buf[AES_BLOCK_SIZE];
    unsigned int num;

    // Some stuff is zeroed.
    memset(ecount_buf, '\0', sizeof(ecount_buf));
    num = 0;

    // Store the initvec; see openssl AES code for details.
    memcpy(ivec, i_ivptr, 8);

    // How many blocks in are we?
    uint64 nblks = i_offset / AES_BLOCK_SIZE;

    // Store the block count in ivec; see openssl AES code for details.
#if defined (ACE_LITTLE_ENDIAN)
    nblks = bswap_64(nblks);
#endif
    memcpy(ivec + 8, &nblks, 8);

    AES_ctr128_encrypt(in, out, i_size, &m_key, ivec, ecount_buf, &num);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
