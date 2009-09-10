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

#if 0
    // FIXME - This all bogus bootstrapping config ...
    {
        BlockStoreConfig * bscfg = m_asscfg.add_blockstore();
        bscfg->set_name("local");
        bscfg->set_type("FSBS");
        bscfg->add_arg("local.bs");
    }

    {
        BlockStoreConfig * bscfg = m_asscfg.add_blockstore();
        bscfg->set_name("other");
        bscfg->set_type("FSBS");
        bscfg->add_arg("other.bs");
    }

    {
        BlockStoreConfig * bscfg = m_asscfg.add_blockstore();
        bscfg->set_name("remote");
        bscfg->set_type("S3BS");
        bscfg->add_arg("--s3-access-key-id=xyzzy");
        bscfg->add_arg("--s3-secret-access-key=blorkblorkblork");
        bscfg->add_arg("--bucket=ksedgwic-utopfs-bs1");
    }

    {
        BlockStoreConfig * bscfg = m_asscfg.add_blockstore();
        bscfg->set_name("rootbs");
        bscfg->set_type("VBS");
        bscfg->add_arg("local");
        bscfg->add_arg("other");
        bscfg->add_arg("remote");
    }

    FileSystemConfig * fscfg = m_asscfg.mutable_filesystem();
    fscfg->set_bsname("rootbs");
    fscfg->set_fsid("MyFileSystem");
    fscfg->set_passphrase("xyzzy");
    fscfg->set_uname("ksedgwic");
    fscfg->set_gname("ksedgwic");
#endif
}

Assembly::~Assembly()
{
    LOG(lgr, 4, "DTOR");
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
