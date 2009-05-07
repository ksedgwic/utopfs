#include <sstream>
#include <string>

#include "Log.h"

#include "FSBlockStore.h"
#include "fsbslog.h"

using namespace std;
using namespace utp;

namespace FSBS {

FSBlockStore::FSBlockStore()
{
    LOG(lgr, 4, "CTOR");
}

FSBlockStore::~FSBlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
FSBlockStore::bs_create(std::string const & i_path)
    throw(InternalError,
          ValueError)
{
    LOG(lgr, 4, "bs_create " << i_path);

    m_path = i_path;
    mkdir(m_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
}

void
FSBlockStore::bs_open(std::string const & i_path)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "bs_open " << i_path);

    m_path = i_path;
}

void
FSBlockStore::bs_close()
    throw(InternalError)
{
    LOG(lgr, 4, "bs_close");

//    throwstream(InternalError, FILELINE
//                << "FSBlockStore::bs_close unimplemented");
}

size_t
FSBlockStore::bs_get_block(void const * i_keydata,
                           size_t i_keysize,
                           void * o_outbuff,
                           size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
          
{
    LOG(lgr, 6, "bs_get_block");

    // convert key to filesystem-safe name
    //std::string s_filename = new std::string(
    
    std::string s_filename = m_path + "/testblock";
    
    struct stat statbuff;
    stat(s_filename.c_str(), &statbuff);
    
    if (statbuff.st_size > i_outsize) {
        throwstream(InternalError, FILELINE
                << "Passed buffer not big enough to hold block");
    }
    
    int fd = open(s_filename.c_str(), O_RDONLY, S_IRUSR);
    int bytes_read = read(fd,o_outbuff,statbuff.st_size);
    close(fd);
        
    if (bytes_read == 0) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_get_block got EOF...");
    }
    
    return bytes_read;
}

void
FSBlockStore::bs_put_block(void const * i_keydata,
                           size_t i_keysize,
                           void const * i_blkdata,
                           size_t i_blksize)
    throw(InternalError,
          ValueError)
{
    LOG(lgr, 6, "bs_put_block");

    std::string s_filename = m_path + "/testblock";
    
    int fd = open(s_filename.c_str(),O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
    if (fd < 0) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::can't get file descriptor..." << s_filename);
    }
    
    int bytes_written = write(fd,i_blkdata,i_blksize);    
    close(fd);
        
    if (bytes_written == -1) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_put_block write error...");
    }
}

void
FSBlockStore::bs_del_block(void const * i_keydata,
                           size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 6, "bs_del_block");
    
    std::string s_filename = m_path + "/testblock";
    unlink(s_filename.c_str());
    throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_del_block unimplemented");
}

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
