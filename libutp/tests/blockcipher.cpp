#include <cstring>
#include <iostream>
#include <string>

#include "Base32.h"
#include "Random.h"
#include "BlockCipher.h"

using namespace utp;
using namespace std;

int
main(int argc, char ** argv)
{
    string secret = "Shhhh!";

    BlockCipher cipher((uint8 const *) secret.c_str(), secret.size());

    uint8 iv[16];
    Random::fill(iv, sizeof(iv));

    char buffer[8192];
    memset(buffer, '\0', sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "This is a secret message");

    cipher.encrypt(iv, buffer, sizeof(buffer));

    cipher.decrypt(iv, buffer, sizeof(buffer));

    cout << buffer << endl;

    return 0;
}
