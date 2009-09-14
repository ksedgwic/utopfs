#include <sstream>
#include <string>

#include <db_cxx.h>

#include "Base32.h"
#include "BlockStoreFactory.h"
#include "Log.h"
#include "Scoped.h"

#include "BDBBlockStore.h"
#include "bdbbslog.h"


using namespace std;
using namespace utp;

namespace BDBBS {

//for the scoped db objects
void dbe_delete(DbEnv * dbp) {
	delete dbp;
}

void db_delete(Db * dbp) {
	delete dbp;
}

void dbe_close(DbEnv * dbp) {
	dbp->close(0);
	delete dbp;
}
void db_close(Db * dbp) {
	dbp->close(0);
	delete dbp;
}

void
BDBBlockStore::destroy(StringSeq const & i_args)
{
	LOG(lgr,4,"Destroy on" << i_args[0]);

	ACE_stat sb;
	
	string const & path = i_args[0];
	
    // Make sure the top level is a directory.
	if (ACE_OS::stat(path.c_str(), &sb) != 0)
		throwstream(NotFoundError, "BDBBlockStore::destroy: top dir \""
			<< path << "\" does not exist");

	string cmd = "rm -rf " + path;
	system(cmd.c_str());
}

BDBBlockStore::BDBBlockStore(string const & i_instname)
    : m_instname(i_instname)
{	
    LOG(lgr, 4, m_instname << ' ' << "CTOR");
	m_db_opened = false;
}

BDBBlockStore::~BDBBlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

string const &
BDBBlockStore::bs_instname() const
{
    return m_instname;
}

void
BDBBlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    string const & path = i_args[0];

    LOG(lgr, 4, m_instname << ' ' << "bs_create " << i_size << ' ' << path);

	m_rootpath = path;

	struct stat statbuff;    
    if (stat(m_rootpath.c_str(),&statbuff) == 0) {
         throwstream(NotUniqueError, FILELINE
                << "Cannot create bdb block store at '" << m_rootpath << "'. File or directory already exists.");   
    }
	
    // Make the parent directory.
    if (mkdir(m_rootpath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        throwstream(InternalError, FILELINE
                    << "mkdir " << m_rootpath << "failed: "
                    << ACE_OS::strerror(errno));
	
	open_dbs(DB_CREATE);
	 
}

void
BDBBlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    if (i_args.empty())
        throwstream(ValueError, "missing blockstore path argument");

    string const & path = i_args[0];

    LOG(lgr, 4, m_instname << ' ' << "bs_open " << path);	

    struct stat statbuff;    
    if (stat(path.c_str(), &statbuff) != 0) {
        throwstream(NotFoundError, FILELINE
                << "Cannot open bdb block store at '" << path << "'. File does not exist.");    
    }  

	m_rootpath = path;
	std::string db_path = "main";
	
	open_dbs(0);

}

void
BDBBlockStore::bs_close()
    throw(InternalError)
{
    LOG(lgr, 4, m_instname << ' ' << "bs_close");

    // Unregister this instance.
    try
    {
        BlockStoreFactory::unmap(m_instname);
    }
    catch (InternalError const & ex)
    {
        // This we can just rethrow ...
        throw;
    }
    catch (Exception const & ex)
    {
        // These shouldn't happen and need to be converted to
        // InternalError ...
        throw InternalError(ex.what());
    }

	if (m_db_opened) {		
		try {
			m_db->close(0);
			delete(m_db);
			m_db_refresh_ids->close(0);
			delete(m_db_refresh_ids);
			m_db_refresh_entries->close(0);
			delete(m_db_refresh_entries);
			m_dbe->close(0);	
			delete(m_dbe);
		} catch (DbException e) {
			LOG(lgr, 1, FILELINE
                << m_instname << ' ' << "BDBBlockStore error on close" 
                << ": error: " << ACE_OS::strerror(errno));
		}
	}	
	m_db_opened = false;
}

void
BDBBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_stat unimplemented");
}

void
BDBBlockStore::bs_sync()
    throw(InternalError)
{
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}

	m_db->sync(0);
	m_db_refresh_ids->sync(0);
	m_db_refresh_entries->sync(0);
}

#if 0
size_t
BDBBlockStore::bs_block_get(void const * i_keydata,
                           size_t i_keysize,
                           void * o_outbuff,
                           size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
          
{
    LOG(lgr, 6, m_instname << ' ' << "bs_block_get");
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}
        
    Dbt key((void *)i_keydata,i_keysize);
    Dbt data;    
    data.set_data(o_outbuff);
    data.set_ulen(i_outsize);
    data.set_flags(DB_DBT_USERMEM);
    
	
    int result = m_db->get(NULL,&key,&data,0);
    if (result == DB_NOTFOUND) {
        throwstream(NotFoundError, FILELINE);
    } else if (result != 0) {
        throwstream(NotFoundError, FILELINE
                << "BDBBlockStore::bs_block_get: " << result << db_strerror(result));
	}	
    
    return data.get_size();    
}
#endif

void
 BDBBlockStore::bs_block_get_async(void const * i_keydata,
                                  size_t i_keysize,
                                  void * o_buffdata,
                                  size_t i_buffsize,
                                  BlockGetCompletion & i_cmpl,
                                  void const * i_argp)
    throw(InternalError,
          ValueError)
{
    try
    {
        LOG(lgr, 6, m_instname << ' ' << "bs_block_get");
        if (! m_db_opened) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore db not opened!");
        }
        
        Dbt key((void *)i_keydata,i_keysize);
        Dbt data;    
        data.set_data(o_buffdata);
        data.set_ulen(i_buffsize);
        data.set_flags(DB_DBT_USERMEM);
    
	
        int result = m_db->get(NULL,&key,&data,0);
        if (result == DB_NOTFOUND) {
            throwstream(NotFoundError, FILELINE);
        } else if (result != 0) {
            throwstream(NotFoundError, FILELINE
                        << "BDBBlockStore::bs_block_get: " << result << db_strerror(result));
        }	
    
        i_cmpl.bg_complete(i_keydata, i_keysize, i_argp, data.get_size());
    }
    catch (Exception const & ex)
    {
        i_cmpl.bg_error(i_keydata, i_keysize, i_argp, ex);
    }
}

#if 0
void
BDBBlockStore::bs_block_put(void const * i_keydata,
                           size_t i_keysize,
                           void const * i_blkdata,
                           size_t i_blksize)
    throw(InternalError,
          ValueError,
          NoSpaceError)
{
    LOG(lgr, 6, m_instname << ' ' << "bs_block_put");
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}
    
    Dbt key((void *)i_keydata,i_keysize);
    Dbt data((void *)i_blkdata,i_blksize);
    
    //delete key if it exists--necessary for root-node persistence
    if (m_db->exists(NULL,&key,0) == 0) {
    	m_db->del(NULL,&key,0); 
    }    
    int results = m_db->put(NULL,&key,&data,DB_NOOVERWRITE);
    if (results != 0) {
    	throwstream(InternalError, FILELINE
                << "BDBBlockStore::bs_block_put returned error " << results << db_strerror(results));
    }      
}
#endif

void
BDBBlockStore::bs_block_put_async(void const * i_keydata,
                                  size_t i_keysize,
                                  void const * i_blkdata,
                                  size_t i_blksize,
                                  BlockPutCompletion & i_cmpl,
                                  void const * i_argp)
    throw(InternalError,
          ValueError)
{
    try
    {
        LOG(lgr, 6, m_instname << ' ' << "bs_block_put");
        if (! m_db_opened) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore db not opened!");
        }
    
        Dbt key((void *)i_keydata,i_keysize);
        Dbt data((void *)i_blkdata,i_blksize);
    
        //delete key if it exists--necessary for root-node persistence
        if (m_db->exists(NULL,&key,0) == 0) {
            m_db->del(NULL,&key,0); 
        }    
        int results = m_db->put(NULL,&key,&data,DB_NOOVERWRITE);
        if (results != 0) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore::bs_block_put returned error " << results << db_strerror(results));
        }      

        i_cmpl.bp_complete(i_keydata, i_keysize, i_argp);
    }
    catch (Exception const & ex)
    {
        i_cmpl.bp_error(i_keydata, i_keysize, i_argp, ex);
    }
}

void
BDBBlockStore::bs_refresh_start_async(uint64 i_rid,
                                RefreshStartCompletion & i_cmpl,
                                void const * i_argp)
    throw(InternalError)
{
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}	

    try
    {
        try {
		
            Dbt rid_key((void *)&i_rid,sizeof(i_rid));

            int results = m_db_refresh_ids->exists(NULL,&rid_key,0);
            if (results == 0) {
                throwstream(NotUniqueError,
                            "refresh id " << i_rid << " already exists!");
            } else if (results != DB_NOTFOUND) { 
                throwstream(InternalError, FILELINE
                            << "BDBBlockStore::bs_refresh_start: " << results << db_strerror(results));
            }

            Dbt key((void *)&i_rid,sizeof(i_rid));
            //data should be date
            Dbt data((void *)&i_rid,sizeof(i_rid));

            results = m_db_refresh_ids->put(NULL,&key,&data,DB_NOOVERWRITE);
            if (results != 0) {
                throwstream(InternalError, FILELINE
                            << "BDBBlockStore::bs_refresh_start returned error " << results << db_strerror(results));
            }

            i_cmpl.rs_complete(i_rid, i_argp);

        } catch (DbException e) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore::bs_refresh_start"
                        << ": error: " << ACE_OS::strerror(errno)); 
        }
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rs_error(i_rid, i_argp, i_ex);
    }
}

#if 0
void
BDBBlockStore::bs_refresh_blocks(uint64 i_rid,
                                 KeySeq const & i_keys,
                                 KeySeq & o_missing)
    throw(InternalError,
          NotFoundError)
{
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}

	int results;
	try {
		//verify that i_rid exists
		Dbt rid_key((void *)&i_rid,sizeof(i_rid));

		results = m_db_refresh_ids->exists(NULL,&rid_key,0);
		if (results == DB_NOTFOUND) {
			throwstream(NotFoundError,
		                "refresh id " << i_rid << " doesn't exist");
		} else if (results != 0) {
		    throwstream(InternalError, FILELINE
		            << "BDBBlockStore::bs_refresh_blocks: " << results << db_strerror(results));
		}
		
		//now put blocks
		o_missing.clear();

		for (unsigned i = 0; i < i_keys.size(); ++i)
		{			
			Dbt block_key((void *)&i_keys[i][0],i_keys[i].size());

		    // If the block doesn't exist add it to the missing list.
			if (m_db->exists(NULL,&block_key,0) == DB_NOTFOUND) {
				o_missing.push_back(i_keys[i]);
		        continue;
		    }

			results = m_db_refresh_entries->put(NULL,&rid_key,&block_key,0);
			if (results != 0) {
				throwstream(InternalError, FILELINE
				        << "BDBBlockStore::bs_refresh_blocks returned error " 
						<< results << db_strerror(results));
			}

		}
	} catch(DbException e) {
		throwstream(InternalError, FILELINE
				        << "BDBBlockStore::bs_refresh_blocks returned error " 
						<< ACE_OS::strerror(errno));
	}
}
#endif

void
BDBBlockStore::bs_refresh_block_async(uint64 i_rid,
                                      void const * i_keydata,
                                      size_t i_keysize,
                                      RefreshBlockCompletion & i_cmpl,
                                      void const * i_argp)
    throw(InternalError,
          NotFoundError)
{
	if (! m_db_opened) {
		throwstream(InternalError, FILELINE
                << "BDBBlockStore db not opened!");
	}

	
	Dbt rid_key((void *)&i_rid,sizeof(i_rid));

	int results = m_db_refresh_ids->exists(NULL,&rid_key,0);
	if (results == DB_NOTFOUND) {
		throwstream(NotFoundError,
	                "refresh id " << i_rid << " doesn't exist");
	} else if (results != 0) {
	    throwstream(InternalError, FILELINE
	            << "BDBBlockStore::bs_refresh_blocks: " << results << db_strerror(results));
	}
	
	i_cmpl.rb_complete(i_keydata, i_keysize, i_argp);

    //throwstream(InternalError, FILELINE << "Feature not implemented"); 
}
        
void
BDBBlockStore::bs_refresh_finish_async(uint64 i_rid,
                                       RefreshFinishCompletion & i_cmpl,
                                       void const * i_argp)
    throw(InternalError)
{
    try
    {
        if (! m_db_opened) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore db not opened!");
        }

        Dbt rid_key((void *)&i_rid,sizeof(i_rid));

        int results = m_db_refresh_ids->exists(NULL,&rid_key,0);
        if (results == DB_NOTFOUND) {
            throwstream(NotFoundError,
                        "refresh id " << i_rid << " doesn't exist");
        } else if (results != 0) {
            throwstream(InternalError, FILELINE
                        << "BDBBlockStore::bs_refresh_blocks: " << results << db_strerror(results));
        }

        i_cmpl.rf_complete(i_rid, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.rf_error(i_rid, i_argp, i_ex);
    }
}

void
BDBBlockStore::bs_head_insert_async(SignedHeadEdge const & i_she,
                                    HeadEdgeInsertCompletion & i_cmpl,
                                    void const * i_argp)
    throw(InternalError)
{
    // FIXME - This placeholder just uses the regular blockstore put
    // and get operations to store a single head node.  This is what
    // we used to do and needs to be replaced with a real headnode
    // graph store ...
    try
    {
        HeadEdge he;
        he.ParseFromString(i_she.headedge());
        string const & key = he.fstag();
        string buf;
        i_she.SerializeToString(&buf);
        bs_block_put(key.data(), key.size(), buf.data(), buf.size());
        i_cmpl.hei_complete(i_she, i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_cmpl.hei_error(i_she, i_argp, i_ex);
    }
}

void
BDBBlockStore::bs_head_follow_async(HeadNode const & i_hn,
                                    HeadEdgeTraverseFunc & i_func,
                                    void const * i_argp)
    throw(InternalError)
{
    // FIXME - This placeholder just uses the regular blockstore put
    // and get operations to store a single head node.  This is what
    // we used to do and needs to be replaced with a real headedge
    // graph store ...
    try
    {
        string const & key = i_hn.first;
        uint8 buf[8192];
        size_t sz = bs_block_get(key.data(), key.size(), buf, sizeof(buf));
        SignedHeadEdge she;
        she.ParseFromArray(buf, sz);
        i_func.het_edge(i_argp, she);
        i_func.het_complete(i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_func.het_error(i_argp, i_ex);
    }
}

void
BDBBlockStore::bs_head_furthest_async(HeadNode const & i_hn,
                                      HeadNodeTraverseFunc & i_func,
                                      void const * i_argp)
    throw(InternalError)
{
    // FIXME - This placeholder just uses the regular blockstore put
    // and get operations to store a single head node.  This is what
    // we used to do and needs to be replaced with a real headedge
    // graph store ...
    try
    {
        string const & key = i_hn.first;
        uint8 buf[8192];
        size_t sz = bs_block_get(key.data(), key.size(), buf, sizeof(buf));
        SignedHeadEdge she;
        she.ParseFromArray(buf, sz);
        HeadEdge he;
        he.ParseFromString(she.headedge());
        HeadNode hn = make_pair(he.fstag(), string());
        i_func.hnt_node(i_argp, hn);
        i_func.hnt_complete(i_argp);
    }
    catch (Exception const & i_ex)
    {
        i_func.hnt_error(i_argp, i_ex);
    }
        
}

string 
BDBBlockStore::get_full_path(void const * i_keydata,
                                size_t i_keysize) 
{
    string s_filename;
    Base32::encode((uint8 const *)i_keydata,i_keysize,s_filename);
    return m_rootpath + "/" + s_filename;
}       


void 
BDBBlockStore::open_dbs(u_int32_t create_flag = 0) 
	throw(InternalError)
{
	//Open the bdb environment for transactions
	u_int32_t flags = DB_INIT_TXN | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_LOG | create_flag;


	Scoped<DbEnv *> dbe(NULL,dbe_delete);
	Scoped<Db *> db(NULL,db_delete);
	Scoped<Db *> db_refresh_ids(NULL,db_delete);
	Scoped<Db *> db_refresh_entries(NULL,db_delete);
	
	Scoped<DbEnv *> odbe(NULL,dbe_close);
	Scoped<Db *> odb(NULL,db_close);
	Scoped<Db *> odb_refresh_ids(NULL,db_close);
	Scoped<Db *> odb_refresh_entries(NULL,db_close);

	std::string db_path;

	try {
		dbe = new DbEnv(0);
		dbe->open(m_rootpath.c_str(),flags ,0);
		odbe = dbe.take();

		db = new Db(dbe,0);
		db_refresh_ids = new Db(dbe,0);
		db_refresh_entries = new Db(dbe,0);
		
		db_path = m_rootpath + "/main.db";
		int result = db->open(NULL,db_path.c_str(),NULL, DB_BTREE,create_flag,0);		
		if (result != 0) {
			throwstream(InternalError, FILELINE
                << "Cannot open  bdb block store at '" << m_rootpath << "/" << db_path
                << ": error: " << result << db_strerror(result));		 
        }
		odb = db.take();

		//open other dbs too
		db_path = m_rootpath + "/refresh.db";
		result = db_refresh_ids->open(NULL,db_path.c_str(),NULL, DB_BTREE, create_flag,0);		
		if (result != 0) {
			throwstream(InternalError, FILELINE
                << "Cannot open  bdb block store at '" << m_rootpath << "/" << db_path
                << ": error: " << result << db_strerror(result));		 
        }
		odb_refresh_ids = db_refresh_ids.take();

		//open other dbs too
		db_path = m_rootpath + "/refresh_entries.db";
		result = db_refresh_entries->open(NULL,db_path.c_str(),NULL, DB_BTREE,create_flag,0);		
		if (result != 0) {
			throwstream(InternalError, FILELINE
                << "Cannot open  bdb block store at '" << m_rootpath << "/" << db_path
                << ": error: " << result << db_strerror(result));		 
        }
        odb_refresh_entries = db_refresh_entries.take();
	} catch (DbException e) {	
		throwstream(InternalError, FILELINE
                << "Cannot open bdb block store at '" << m_rootpath << "/" << db_path
                << ": error: " << ACE_OS::strerror(errno)); 
	}   

	m_dbe = odbe.take();
	m_db = odb.take();
	m_db_refresh_ids = odb_refresh_ids.take();
	m_db_refresh_entries = odb_refresh_entries.take();	
	m_db_opened = true;                
}
} // namespace BDBBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
