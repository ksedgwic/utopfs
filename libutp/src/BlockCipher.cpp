#include <ace/OS_NS_string.h>
#include <ace/os_include/os_byteswap.h>

#include "Except.h"
#include "BlockCipher.h"

using namespace std;
using namespace utp;

namespace utp {

BlockCipher::BlockCipher()
    : m_isvalid(false)
{
}

BlockCipher::BlockCipher(uint8 const * i_keyp, size_t i_keysz)
{
    set_key(i_keyp, i_keysz);
}

BlockCipher::~BlockCipher()
{
    unset_key();
}

void
BlockCipher::set_key(uint8 const * i_keyp, size_t i_keysz)
{
    uint8 buffer[AES_BLOCK_SIZE];
    ACE_OS::memset(buffer, '\0', sizeof(buffer));
    ACE_OS::memcpy(buffer, i_keyp, min(i_keysz, sizeof(buffer)));
    AES_set_encrypt_key(buffer, 128, &m_enckey);
    AES_set_decrypt_key(buffer, 128, &m_deckey);
    m_isvalid = true;
}

void
BlockCipher::unset_key()
{
    // Set the key to all zeros.
    uint8 buffer[AES_BLOCK_SIZE];
    ACE_OS::memset(buffer, '\0', sizeof(buffer));
    AES_set_encrypt_key(buffer, 128, &m_enckey);
    AES_set_decrypt_key(buffer, 128, &m_deckey);
    m_isvalid = false;
}

void
BlockCipher::encrypt(void const * i_ivptr,
                     void * io_data,
                     size_t i_size)
{
    // Looks like the IV may get scribbled on, make a copy.
    uint8 iv[AES_BLOCK_SIZE];
    ACE_OS::memcpy(iv, i_ivptr, sizeof(iv));

    AES_cbc_encrypt((uint8 const *) io_data,
                    (uint8 *) io_data,
                    i_size,
                    &m_enckey,
                    iv,
                    AES_ENCRYPT);
}

void
BlockCipher::decrypt(void const * i_ivptr,
                     void * io_data,
                     size_t i_size)
{
    // Looks like the IV may get scribbled on, make a copy.
    uint8 iv[AES_BLOCK_SIZE];
    ACE_OS::memcpy(iv, i_ivptr, sizeof(iv));

    AES_cbc_encrypt((uint8 const *) io_data,
                    (uint8 *) io_data,
                    i_size,
                    &m_deckey,
                    iv,
                    AES_DECRYPT);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
