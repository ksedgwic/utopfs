#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"

#include "FSBlockStore.h"

#include "FSBSSvc.h"
#include "fsbslog.h"

using namespace std;
using namespace utp;

FSBSSvc::FSBSSvc()
{
}

FSBSSvc::~FSBSSvc()
{
}

int
FSBSSvc::init(int argc, char * argv[])
{
    // Create a new instance and register as the singleton.
    BlockStore::instance(new FSBS::FSBlockStore());

    return 0;
}

int
FSBSSvc::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
FSBSSvc::suspend()
{
    return 0;
}

int
FSBSSvc::resume()
{
    return 0;
}

ACE_SVC_FACTORY_DEFINE(FSBSSvc)

ACE_STATIC_SVC_DEFINE(FSBSSvc,
                      ACE_TEXT ("FSBSSvc"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (FSBSSvc),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
