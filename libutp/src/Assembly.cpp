#include <iostream>
#include <stdexcept>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "BlockStoreFactory.h"
#include "BlockStore.h"
#include "Except.h"
#include "FileSystemFactory.h"
#include "FileSystem.h"

#include "Assembly.h"
#include "Assembly.pb.h"
#include "utplog.h"

using namespace std;
using namespace google::protobuf;
using namespace google::protobuf::io;

namespace utp {

Assembly::Assembly(istream & i_cfgstrm)
{
    LOG(lgr, 4, "CTOR");

    IstreamInputStream iis(&i_cfgstrm);

    // Parse the Assembly config.
    if (!TextFormat::Parse(&iis, &m_asscfg))
        throwstream(ParseError, "parsing of Assembly config failed");

    // Open each blockstore.
    for (int ii = 0; ii < m_asscfg.blockstore_size(); ++ii)
    {
        BlockStoreConfig const & bscfg = m_asscfg.blockstore(ii);
        StringSeq bsargs;
        for (int jj = 0; jj < bscfg.arg_size(); ++jj)
            bsargs.push_back(bscfg.arg(jj));
        BlockStoreFactory::open(bscfg.type(), bscfg.name(), bsargs);
    }

    // Find the root blockstore.

    // Mount the FileSystem.
    FileSystemConfig const & fscfg = m_asscfg.filesystem();
    m_bsh = BlockStoreFactory::lookup(fscfg.bsname());
    StringSeq fsargs;
    for (int jj = 0; jj < fscfg.arg_size(); ++jj)
        fsargs.push_back(fscfg.arg(jj));
    m_fsh = FileSystemFactory::mount(fscfg.type(),
                                     m_bsh,
                                     fscfg.fsid(),
                                     fscfg.passphrase(),
                                     fsargs);
}

Assembly::~Assembly()
{
    LOG(lgr, 4, "DTOR");
}

void
Assembly::get_stats(StatSet & o_ss) const
    throw(InternalError)
{
    o_ss.set_name("");

    {
        StatSet * ssp = o_ss.add_subset();
        m_bsh->bs_get_stats(*ssp);
    }

    {
        StatSet * ssp = o_ss.add_subset();
        m_fsh->fs_get_stats(*ssp);
    }
}

void
Assembly::export_config(ostream & i_ostrm) const
{
    OstreamOutputStream oos(&i_ostrm);
    TextFormat::Print(m_asscfg, &oos);
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
