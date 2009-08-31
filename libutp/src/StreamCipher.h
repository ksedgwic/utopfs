#ifndef utp_StreamCipher_h__
#define utp_StreamCipher_h__

#include <openssl/aes.h>

#include "utpexp.h"
#include "utpfwd.h"

#include "Types.h"

namespace utp {

class UTP_EXP StreamCipher
{
public:
    /// Default constructor.
    StreamCipher();

    /// Create a StreamCipher with a key.
    ///
    /// The first 16 bytes of key data will be used.
    ///
    /// @param[in] i_keyp Pointer to key data.
    /// @param[in] i_keysz Size of key in bytes.
    ///
    StreamCipher(uint8 const * i_keyp, size_t i_keysz);

    /// Destructor.
    ///
    ~StreamCipher();

    /// Set the stream cipher's key.
    ///
    /// @param[in] i_keyp Pointer to key data.
    /// @param[in] i_keysz Size of key in bytes.
    ///
    void set_key(uint8 const * i_keyp, size_t i_keysz);

    /// Unset the stream cipher's key.
    ///
    void unset_key();

    /// Encrypt/Decrypt a data buffer in-place.
    ///
    /// @note The i_offset value must be aligned on the AES block size
    ///       (16 bytes).
    ///
    /// @param[in] i_ivptr Pointer to 8 byte init vector.
    /// @param[in] i_offset Stream offset in bytes.
    /// @param[in,out] io_data Pointer to input/output buffer.
    /// @param[in] i_size Number of bytes to encrypt/decrypt.
    ///
    void encrypt(uint8 const * i_ivptr,
                 size_t i_offset,
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

#endif // utp_StreamCipher_h__
