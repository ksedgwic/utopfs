#ifndef utp_BlockCipher_h__
#define utp_BlockCipher_h__

#include <openssl/aes.h>

#include "Types.h"

namespace utp {

class BlockCipher
{
public:
    /// Default constructor.
    BlockCipher();

    /// Create a BlockCipher with a key.
    ///
    /// The first 16 bytes of key data will be used.
    ///
    /// @param[in] i_keyp Pointer to key data.
    /// @param[in] i_keysz Size of key in bytes.
    ///
    BlockCipher(uint8 const * i_keyp, size_t i_keysz);

    /// Destructor.
    ///
    ~BlockCipher();

    /// Set the cipher's key.
    ///
    /// @param[in] i_keyp Pointer to key data.
    /// @param[in] i_keysz Size of key in bytes.
    ///
    void set_key(uint8 const * i_keyp, size_t i_keysz);

    /// Unset the stream cipher's key.
    ///
    void unset_key();

    /// Encrypt a data buffer in-place.
    ///
    /// @param[in] i_ivptr Pointer to 16 byte init vector.
    /// @param[in,out] io_data Pointer to input/output buffer.
    /// @param[in] i_size Number of bytes to encrypt/decrypt.
    ///
    void encrypt(uint8 const * i_ivptr,
                 uint8 * io_data,
                 size_t i_size);

    /// Decrypt a data buffer in-place.
    ///
    /// @param[in] i_ivptr Pointer to 16 byte init vector.
    /// @param[in,out] io_data Pointer to input/output buffer.
    /// @param[in] i_size Number of bytes to encrypt/decrypt.
    ///
    void decrypt(uint8 const * i_ivptr,
                 uint8 * io_data,
                 size_t i_size);

private:
    bool				m_isvalid;
    AES_KEY				m_key;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_BlockCipher_h__
