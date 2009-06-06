#include <sstream>
#include <string>

#include "Log.h"

#include "FSBlockStore.h"
#include "fsbslog.h"

#include "Base32.h"

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
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    LOG(lgr, 4, "bs_create " << i_path);

    struct stat statbuff;    
    if (stat(i_path.c_str(),&statbuff) == 0) {
         throwstream(NotUniqueError, FILELINE
                << "Cannot create file block store at '" << i_path << "'. File or directory already exists.");   
    }

    m_path = i_path;
    mkdir(m_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    
    
    //check for errors; exception?
}

void
FSBlockStore::bs_open(std::string const & i_path)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "bs_open " << i_path);

    struct stat statbuff;
    
    if (stat(i_path.c_str(), &statbuff) != 0) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << i_path << "'. Directory does not exist.");    
    }  
    
    if (! S_ISDIR(statbuff.st_mode)) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << i_path << "'. Path is not a directory.");
    }

    // Need to generate an error if any needed state (in the
    // directory) is not happy ...

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
    
    //std::string s_filename = m_path + "/testblock";
    std::string s_filename = get_full_path(i_keydata,i_keysize);
    
    struct stat statbuff;
    
    int rv = stat(s_filename.c_str(), &statbuff);
    if (rv == -1)
    {
        if (errno == ENOENT)
            throwstream(NotFoundError,
                        Base32::encode(i_keydata, i_keysize) << ": not found");
        else
            throwstream(InternalError, FILELINE
                        << "FSBlockStore::bs_get_block: "
                        << Base32::encode(i_keydata, i_keysize)
                        << ": error: " << ACE_OS::strerror(errno));
    }
    
    if (statbuff.st_size > off_t(i_outsize)) {
        throwstream(InternalError, FILELINE
                << "Passed buffer not big enough to hold block");
    }
    
    int fd = open(s_filename.c_str(), O_RDONLY, S_IRUSR);
    int bytes_read = read(fd,o_outbuff,statbuff.st_size);
    close(fd);

    if (bytes_read == -1) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_get_block: read failed: " << strerror(errno));
    }

        
    if (bytes_read != statbuff.st_size) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::expected to get " << statbuff.st_size << " bytes, but got " << bytes_read << " bytes");
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

    std::string s_filename = get_full_path(i_keydata,i_keysize);    
    
    int fd = open(s_filename.c_str(),O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
    if (fd == -1) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_put_block: open failed on " << s_filename << ": " << strerror(errno));
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
    
    std::string s_filename = get_full_path(i_keydata,i_keysize);

    unlink(s_filename.c_str());
}

std::string 
FSBlockStore::get_full_path(void const * i_keydata,
                                size_t i_keysize) 
{
    std::string s_filename;
    Base32::encode((uint8 const *)i_keydata,i_keysize,s_filename);
    return m_path + "/" + s_filename;
}                                

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
