#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

#include <cassert>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "Log.h"
#include "Except.h"

#include "BlockStore.h"
#include "Random.h"
#include "StreamCipher.h"

#include "utfslog.h"

#include "Context.h"
#include "FileNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {

string myuname()
{
    struct passwd pw;
    struct passwd * pwp;
    char buf[1024];
    int rv = getpwuid_r(getuid(), &pw, buf, sizeof(buf), &pwp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getpwuid_r failed: " << strerror(rv));
    if (!pwp)
        throwstream(InternalError, FILELINE << "no password entry found");

    return pwp->pw_name;
}

string mygname()
{
    struct group gr;
    struct group * grp;
    char buf[1024];
    int rv = getgrgid_r(getgid(), &gr, buf, sizeof(buf), &grp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getgrgid_r failed: " << strerror(rv));
    if (!grp)
        throwstream(InternalError, FILELINE << "no group entry found");

    return grp->gr_name;
}

} // end namespace

namespace UTFS {

FileNode::FileNode()
{
    LOG(lgr, 4, "CTOR");

    T64 now = T64::now();

    Random::fill(m_initvec, sizeof(m_initvec));

    m_inode.set_mode(S_IRUSR | S_IWUSR | S_IRGRP | S_IFREG);
    m_inode.set_uname(myuname());
    m_inode.set_gname(mygname());
    m_inode.set_size(0);
    m_inode.set_atime(now.usec());
    m_inode.set_mtime(now.usec());
    m_inode.set_ctime(now.usec());

    ACE_OS::memset(m_data, '\0', sizeof(m_data));
}

FileNode::FileNode(Context & i_ctxt, Digest const & i_dig)
    : m_digest(i_dig)
{
    LOG(lgr, 6, "CTOR " << i_dig);

    uint8 buf[BLKSZ];

    // Read the block from the blockstore.
    i_ctxt.m_bsh->bs_get_block(i_dig.data(), i_dig.size(),
                               buf, sizeof(buf));

    // The initvec is not encrypted.
    ACE_OS::memcpy(m_initvec, buf, sizeof(m_initvec));

    // Decrypt the block.
    i_ctxt.m_cipher.encrypt(m_initvec, 0, buf, sizeof(buf));

    // Compute the size of fixed fields after the inode.
    size_t fixedsz = fixed_field_size();

    // Parse the INode data.
    size_t sz = BLKSZ - sizeof(m_initvec) - fixedsz;
    
    // NOTE - We use protobuf components individually here
    // because ParseFromArray insists that the entire input
    // stream be consumed.
    //
    ArrayInputStream input(buf + sizeof(m_initvec), sz);
    CodedInputStream decoder(&input);
    if (!m_inode.ParseFromCodedStream(&decoder))
        throwstream(InternalError, FILELINE
                    << "inode deserialization failed");

    // Setup a pointer for the fixed fields.
    uint8 * ptr = &buf[BLKSZ - fixedsz];

    // Copy the inline data.
    ACE_OS::memcpy(m_data, ptr, sizeof(m_data));
    ptr += sizeof(m_data);

    // Copy the block references.
    ACE_OS::memcpy(m_direct, ptr, sizeof(m_direct));
    ptr += sizeof(m_direct);

    ACE_OS::memcpy(&m_sindir, ptr, sizeof(m_sindir));
    ptr += sizeof(m_sindir);

    ACE_OS::memcpy(&m_dindir, ptr, sizeof(m_dindir));
    ptr += sizeof(m_dindir);
    
    ACE_OS::memcpy(&m_tindir, ptr, sizeof(m_tindir));
    ptr += sizeof(m_tindir);
    
    ACE_OS::memcpy(&m_qindir, ptr, sizeof(m_qindir));
    ptr += sizeof(m_qindir);

#ifdef DEBUG
    assert(ptr == buf + BLKSZ);
#endif
}

FileNode::~FileNode()
{
    LOG(lgr, 6, "DTOR");
}

void
FileNode::persist(Context & i_ctxt)
{
    uint8 buf[BLKSZ];

    // Clear the buffer first.  Seems unnecessary, but I don't
    // like leaking our stack contents out in blocks ...
    //
    ACE_OS::memset(buf, '\0', sizeof(buf));

    // Compute the size of fixed fields after the inode.
    size_t fixedsz = fixed_field_size();
    
    // Compute the size available for the inode fields and serialize.
    size_t sz = BLKSZ - sizeof(m_initvec) - fixedsz;
    bool rv = m_inode.SerializeToArray(buf + sizeof(m_initvec), sz);
    if (!rv)
        throwstream(InternalError, FILELINE << "inode serialization error");

    // Setup a pointer for the fixed fields.
    uint8 * ptr = &buf[BLKSZ - fixedsz];

    // Copy the inline data.
    ACE_OS::memcpy(ptr, m_data, sizeof(m_data));
    ptr += sizeof(m_data);

    // Copy the block references.
    ACE_OS::memcpy(ptr, m_direct, sizeof(m_direct));
    ptr += sizeof(m_direct);

    ACE_OS::memcpy(ptr, &m_sindir, sizeof(m_sindir));
    ptr += sizeof(m_sindir);

    ACE_OS::memcpy(ptr, &m_dindir, sizeof(m_dindir));
    ptr += sizeof(m_dindir);
    
    ACE_OS::memcpy(ptr, &m_tindir, sizeof(m_tindir));
    ptr += sizeof(m_tindir);
    
    ACE_OS::memcpy(ptr, &m_qindir, sizeof(m_qindir));
    ptr += sizeof(m_qindir);

#ifdef DEBUG
    assert(ptr == buf + BLKSZ);
#endif

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(m_initvec, 0, buf, sizeof(buf));

    // Write the initvec in the front.
    ACE_OS::memcpy(buf, m_initvec, sizeof(m_initvec));

    // Take the digest of the whole thing.
    m_digest = Digest(buf, sizeof(buf));

    LOG(lgr, 6, "persist " << m_digest);

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(m_digest.data(), m_digest.size(),
                               buf, sizeof(buf));
}

int
FileNode::getattr(Context & i_ctxt, struct stat * o_statbuf)
{
    o_statbuf->st_mode = m_inode.mode();
    o_statbuf->st_uid = 0;    // FIXME - uid missing!
    o_statbuf->st_gid = 0;    // FIXME - gid missing!
    o_statbuf->st_size = m_inode.size();
    o_statbuf->st_atime = m_inode.atime() / 1000000;
    o_statbuf->st_mtime = m_inode.mtime() / 1000000;
    o_statbuf->st_ctime = m_inode.ctime() / 1000000;

    return 0;
}

int
FileNode::read(Context & i_ctxt, void * o_bufptr, size_t i_size, off_t i_off)
{
    // For now, bail on anything that is bigger then fits
    // in a single INode.
    //
    if (i_off + i_size > 2048)
        throwstream(InternalError, FILELINE
                    << "FileNode::read outside first 2048 bytes unsupported");

    ACE_OS::memcpy(o_bufptr, m_data + i_off, i_size);
    return i_size;
}

int
FileNode::write(Context & i_ctxt,
                void const * i_data,
                size_t i_size,
                off_t i_off)
{
    // For now, bail on anything that is bigger then fits
    // in a single INode.
    //
    if (i_off + i_size > 2048)
        throwstream(InternalError, FILELINE
                    << "FileNode::write outside first 2048 bytes unsupported");

    ACE_OS::memcpy(m_data + i_off, i_data, i_size);
    return i_size;
}

size_t
FileNode::fixed_field_size() const
{
    return sizeof(m_data)
        + sizeof(m_direct)
        + sizeof(m_sindir)
        + sizeof(m_dindir)
        + sizeof(m_tindir)
        + sizeof(m_qindir);
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
