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
FSBlockStore::bs_create(size_t i_size, string const & i_path)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    LOG(lgr, 4, "bs_create " << i_size << ' ' << i_path);

    struct stat statbuff;    
    if (stat(i_path.c_str(),&statbuff) == 0) {
         throwstream(NotUniqueError, FILELINE
                << "Cannot create file block store at '" << i_path
                     << "'. File or directory already exists.");   
    }

    m_path = i_path;
    mkdir(m_path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    
    
    //check for errors; exception?
}

void
FSBlockStore::bs_open(string const & i_path)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "bs_open " << i_path);

    struct stat statbuff;
    
    if (stat(i_path.c_str(), &statbuff) != 0) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << i_path
                    << "'. Directory does not exist.");    
    }  
    
    if (! S_ISDIR(statbuff.st_mode)) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << i_path
                    << "'. Path is not a directory.");
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

void
FSBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_stat unimplemented");
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
    //string s_filename = new string(
    
    //string s_filename = m_path + "/testblock";
    string s_filename = blockpath(i_keydata,i_keysize);
    
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
                << "FSBlockStore::bs_get_block: read failed: "
                    << strerror(errno));
    }

        
    if (bytes_read != statbuff.st_size) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::expected to get " << statbuff.st_size
                    << " bytes, but got " << bytes_read << " bytes");
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

    string s_filename = blockpath(i_keydata,i_keysize);    
    
    int fd = open(s_filename.c_str(),
                  O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
    if (fd == -1) {
        throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_put_block: open failed on "
                    << s_filename << ": " << strerror(errno));
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
    
    string s_filename = blockpath(i_keydata,i_keysize);

    unlink(s_filename.c_str());
}

void
FSBlockStore::bs_refresh_start(uint64 i_rid)
    throw(InternalError,
          NotUniqueError)
{
    string rpath = ridpath(i_rid);

    // Does the refresh ID already exist?
    ACE_stat sb;
    int rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv != -1 && errno != ENOENT)
        throwstream(NotUniqueError,
                    "refresh id " << i_rid << " already exists");

    // Create the refresh id mark.
    if (mknod(rpath.c_str(), S_IFREG, 0) != 0)
        throwstream(InternalError,
                    "mknod " << rpath << " failed: "
                    << ACE_OS::strerror(errno));
}

void
FSBlockStore::bs_refresh_blocks(uint64 i_rid,
                                KeySeq const & i_keys,
                                KeySeq & o_missing)
    throw(InternalError,
          NotFoundError)
{
    o_missing.clear();

    string rpath = ridpath(i_rid);

    // Does the refresh ID exist?
    ACE_stat sb;
    int rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv != 0 || !S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "refresh id " << i_rid << " not found");

    for (unsigned i = 0; i < i_keys.size(); ++i)
    {
        string s_filename = blockpath(&i_keys[i][0], i_keys[i].size());

        // If the block doesn't exist add it to the missing list.
        rv = ACE_OS::stat(s_filename.c_str(), &sb);
        if (rv != 0 || !S_ISREG(sb.st_mode))
        {
            o_missing.push_back(i_keys[i]);
            continue;
        }

        // Touch the block.
        rv = utimes(s_filename.c_str(), NULL);
        if (rv != 0)
            throwstream(InternalError, FILELINE
                        << "trouble touching \"" << s_filename
                        << "\": " << ACE_OS::strerror(errno));
    }
}

void
FSBlockStore::bs_refresh_finish(uint64 i_rid)
    throw(InternalError,
          NotFoundError)
{
    string rpath = ridpath(i_rid);

    // Does the refresh ID exist?
    ACE_stat sb;
    int rv = ACE_OS::stat(rpath.c_str(), &sb);
    if (rv != 0 || !S_ISREG(sb.st_mode))
        throwstream(NotFoundError,
                    "refresh id " << i_rid << " not found");

    // Rename the refresh tag to the mark name.
    if (rename(rpath.c_str(), markpath().c_str()) != 0)
        throwstream(InternalError,
                    "rename " << rpath << ' ' << markpath() << " failed:"
                    << ACE_OS::strerror(errno));
}

void
FSBlockStore::bs_sync()
    throw(InternalError)
{
	//always synced to disk
}


string 
FSBlockStore::blockpath(void const * i_keydata, size_t i_keysize) const
{
    string s_filename;
    Base32::encode((uint8 const *)i_keydata,i_keysize,s_filename);
    return m_path + "/" + s_filename;
}                                

string 
FSBlockStore::ridpath(uint64 i_rid) const
{
    ostringstream pathstrm;
    pathstrm << m_path << '/' << "RID:" << i_rid;
    return pathstrm.str();
}                                

string 
FSBlockStore::markpath() const
{
    return m_path + '/' + "MARK";
}                                

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
