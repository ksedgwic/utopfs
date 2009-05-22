#include <cstring>
#include <iostream>
#include <string>

#include "Base32.h"
#include "StreamCipher.h"

using namespace utp;
using namespace std;

int
main(int argc, char ** argv)
{
    string secret = "Shhhh!";

    StreamCipher cipher((uint8 const *) secret.c_str(), secret.size());

    uint8 * msg = (uint8 *) strdup("This is a secret message");
    size_t msglen = strlen((char const *) msg);

    uint8 iv[8];
    memset(iv, '\0', sizeof(iv));

    // Encrypt
    cipher.encrypt(iv, 16, msg, msglen);

    // cout << msg << endl;

    // Decrypt
    cipher.encrypt(iv, 16, msg, msglen);

    cout << msg << endl;

    return 0;
}
