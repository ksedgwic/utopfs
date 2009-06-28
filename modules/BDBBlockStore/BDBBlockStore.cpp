#include <sstream>
#include <string>

#include <db_cxx.h>

#include "Log.h"

#include "BDBBlockStore.h"
#include "bdbbslog.h"

#include "Base32.h"

using namespace std;
using namespace utp;

namespace BDBBS {

BDBBlockStore::BDBBlockStore()
{	
    LOG(lgr, 4, "CTOR");
    db = new Db(NULL,0);
}

BDBBlockStore::~BDBBlockStore()
{
	db->close(0);
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
BDBBlockStore::bs_create(string const & i_path)
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
		
	try {	
		db->open(NULL,i_path.c_str(),NULL, DB_BTREE,DB_CREATE,0);		
	} catch (DbException e) {
		throwstream(InternalError, FILELINE
                << "Cannot create bdb block store at '" << i_path
                << ": error: " << ACE_OS::strerror(errno)); 
	}
	 
}

void
BDBBlockStore::bs_open(string const & i_path)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "bs_open " << i_path);

    struct stat statbuff;    
    if (stat(i_path.c_str(), &statbuff) != 0) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open file block store at '" << i_path << "'. Directory does not exist.");    
    }  


	try {	
		db->open(NULL,i_path.c_str(),NULL, DB_BTREE,0,0);		
	} catch (DbException e) {	
		throwstream(InternalError, FILELINE
                << "Cannot open bdb block store at '" << i_path
                << ": error: " << ACE_OS::strerror(errno)); 
	}

}

void
BDBBlockStore::bs_close()
    throw(InternalError)
{
    LOG(lgr, 4, "bs_close");    
}

size_t
BDBBlockStore::bs_get_block(void const * i_keydata,
                           size_t i_keysize,
                           void * o_outbuff,
                           size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
          
{
    LOG(lgr, 6, "bs_get_block");
    throwstream(InternalError, FILELINE << "Feature not implemented"); 


    // convert key to filesystem-safe name
    //string s_filename = new string(
    
    //string s_filename = m_path + "/testblock";
    string s_filename = get_full_path(i_keydata,i_keysize);
    
    struct stat statbuff;
    
    int rv = stat(s_filename.c_str(), &statbuff);
    if (rv == -1)
    {
        if (errno == ENOENT)
            throwstream(NotFoundError,
                        Base32::encode(i_keydata, i_keysize) << ": not found");
        else
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore::bs_get_block: "
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
                << "BDBBlockStore::bs_get_block: read failed: " << strerror(errno));
    }

        
    if (bytes_read != statbuff.st_size) {
        throwstream(InternalError, FILELINE
                << "BDBBlockStore::expected to get " << statbuff.st_size << " bytes, but got " << bytes_read << " bytes");
    }
    
    return bytes_read;
}

void
BDBBlockStore::bs_put_block(void const * i_keydata,
                           size_t i_keysize,
                           void const * i_blkdata,
                           size_t i_blksize)
    throw(InternalError,
          ValueError)
{
    LOG(lgr, 6, "bs_put_block");
    throwstream(InternalError, FILELINE << "Feature not implemented"); 

    string s_filename = get_full_path(i_keydata,i_keysize);    
    
    int fd = open(s_filename.c_str(),O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);    
    if (fd == -1) {
        throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_put_block: open failed on " << s_filename << ": " << strerror(errno));
    }
    
    int bytes_written = write(fd,i_blkdata,i_blksize);    
    close(fd);
        
    if (bytes_written == -1) {
        throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_put_block write error...");
    }
}

void
BDBBlockStore::bs_del_block(void const * i_keydata,
                           size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 6, "bs_del_block");
    throwstream(InternalError, FILELINE << "Feature not implemented"); 
    
    string s_filename = get_full_path(i_keydata,i_keysize);

    unlink(s_filename.c_str());
}

void
BDBBlockStore::bs_refresh_blocks(KeySeq const & i_keys,
                                KeySeq & o_missing)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE << "Feature not implemented"); 
    o_missing.clear();

    for (unsigned i = 0; i < i_keys.size(); ++i)
    {
        string s_filename = get_full_path(&i_keys[i][0], i_keys[i].size());

        // If the block doesn't exist add it to the missing list.
        ACE_stat sb;
        int rv = ACE_OS::stat(s_filename.c_str(), &sb);
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

string 
BDBBlockStore::get_full_path(void const * i_keydata,
                                size_t i_keysize) 
{
    string s_filename;
    Base32::encode((uint8 const *)i_keydata,i_keysize,s_filename);
    return m_path + "/" + s_filename;
}                                

} // namespace BDBBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
