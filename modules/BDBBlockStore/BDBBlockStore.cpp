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
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
BDBBlockStore::bs_create(size_t i_size, string const & i_path)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    LOG(lgr, 4, "bs_create " << i_size << ' ' << i_path);

	struct stat statbuff;    
    if (stat(i_path.c_str(),&statbuff) == 0) {
         throwstream(NotUniqueError, FILELINE
                << "Cannot create bdb block store at '" << i_path << "'. File or directory already exists.");   
    }
		
	try {	
		int result = db->open(NULL,i_path.c_str(),NULL, DB_BTREE,DB_CREATE,0);		
		if (result != 0) {
			throwstream(InternalError, FILELINE
                << "Cannot create bdb block store at '" << i_path
                << ": error: " << result << db_strerror(result));		 
        }
		 
		 
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
                << "Cannot open bdb block store at '" << i_path << "'. File does not exist.");    
    }  


	try {	
		int result = db->open(NULL,i_path.c_str(),NULL, DB_BTREE,0,0);		
		if (result != 0) {
			throwstream(InternalError, FILELINE
                << "Cannot open  bdb block store at '" << i_path
                << ": error: " << result << db_strerror(result));		 
        }
        
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
    if (db) {
    	db->sync(0);
		db->close(0);
	}
}

void
BDBBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_stat unimplemented");
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
        
    Dbt key((void *)i_keydata,i_keysize);
    Dbt data;    
    data.set_data(o_outbuff);
    data.set_ulen(i_outsize);
    data.set_flags(DB_DBT_USERMEM);
    
    
    int result = db->get(NULL,&key,&data,0);
    if (result != 0) {
        throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_get_block: " << result << db_strerror(result));
    }
    
    return data.get_size();    
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
    
    Dbt key((void *)i_keydata,i_keysize);
    Dbt data((void *)i_blkdata,i_blksize);
    
    //delete key if it exists--necessary for root-node persistence
    if (db->exists(NULL,&key,0) == 0) {
    	db->del(NULL,&key,0); 
    }    
    int results = db->put(NULL,&key,&data,DB_NOOVERWRITE);
    if (results != 0) {
    	throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_put_block returned error " << results << db_strerror(results));
    }      
}

void
BDBBlockStore::bs_del_block(void const * i_keydata,
                           size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 6, "bs_del_block");
    
    Dbt key((void *)i_keydata,i_keysize);
    int results = db->del(NULL,&key,0);    
    if (results != 0) {
    	throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_del_block returned error " << results << db_strerror(results));
    }   
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

void
BDBBlockStore::bs_sync()
    throw(InternalError)
{
	db->sync(0);
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
