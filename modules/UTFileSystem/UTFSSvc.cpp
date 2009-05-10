#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"

#include "UTFileSystem.h"

#include "UTFSSvc.h"
#include "utfslog.h"

using namespace std;
using namespace utp;

UTFSSvc::UTFSSvc()
{
}

UTFSSvc::~UTFSSvc()
{
}

int
UTFSSvc::init(int argc, char * argv[])
{
    // Create a new instance and register as the singleton.
    FileSystem::instance(new UTFS::UTFileSystem());

    return 0;
}

int
UTFSSvc::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
UTFSSvc::suspend()
{
    return 0;
}

int
UTFSSvc::resume()
{
    return 0;
}

ACE_SVC_FACTORY_DEFINE(UTFSSvc)

ACE_STATIC_SVC_DEFINE(UTFSSvc,
                      ACE_TEXT ("UTFSSvc"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (UTFSSvc),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
