#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Scoped.h"

using namespace std;
using namespace utp;

void
myterm(char * ptr)
{
    cerr << "myfree called on " << ptr << endl;
    free(ptr);
}

int
main(int argc, char ** argv)
{
    // This will get freed on exit.
    Scoped<char *> ptr1(strdup("foo"), NULL, myterm);

    // You don't have to initialize immediately.
    Scoped<char *> ptr2(NULL, myterm);
    ptr2 = strdup("bar");

    // You can "take" a value.
    Scoped<char *> ptr3(strdup("blat"), NULL, myterm);
    char * ptrx = ptr3.take();	// Now myterm won't get called.
    free(ptrx); // Need to free it ourselves now.

    return 0;
}
