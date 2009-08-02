#include <iostream>

#include <ace/OS_NS_string.h>

#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "MD5.h"
#include "Except.h"

using namespace std;
using namespace utp;

namespace utp {

MD5::MD5(void const * i_data, size_t i_size)
{
    unsigned char data[16];
    
    ::MD5((unsigned char const *) i_data, i_size, data);

    BIO * bio, * b64;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_write(bio, data, sizeof(data));
    (void) BIO_flush(bio);

    char const * ptr;
    long sz = BIO_get_mem_data(bio, &ptr);
    if (sz > 0)
        m_digstr = string(ptr, sz - 1);

    BIO_free_all(bio);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
