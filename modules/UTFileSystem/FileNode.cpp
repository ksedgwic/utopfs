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

#include "BlockNode.h"
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
    m_inode.set_nlink(1);
    m_inode.set_uname(myuname());
    m_inode.set_gname(mygname());
    m_inode.set_size(0);
    m_inode.set_atime(now.usec());
    m_inode.set_mtime(now.usec());
    m_inode.set_ctime(now.usec());

    ACE_OS::memset(m_inl, '\0', sizeof(m_inl));
}

FileNode::FileNode(Context & i_ctxt, Digest const & i_dig)
    : RefBlockNode(i_dig)
{
    LOG(lgr, 6, "CTOR " << i_dig);

    uint8 buf[BlockNode::BLKSZ];

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
    size_t sz = BlockNode::BLKSZ - sizeof(m_initvec) - fixedsz;
    
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
    uint8 * ptr = &buf[BlockNode::BLKSZ - fixedsz];

    // Copy the inline data.
    ACE_OS::memcpy(m_inl, ptr, sizeof(m_inl));
    ptr += sizeof(m_inl);

    // Copy the block references.
    ACE_OS::memcpy(m_dirref, ptr, sizeof(m_dirref));
    ptr += sizeof(m_dirref);

    ACE_OS::memcpy(&m_sinref, ptr, sizeof(m_sinref));
    ptr += sizeof(m_sinref);

    ACE_OS::memcpy(&m_dinref, ptr, sizeof(m_dinref));
    ptr += sizeof(m_dinref);
    
    ACE_OS::memcpy(&m_tinref, ptr, sizeof(m_tinref));
    ptr += sizeof(m_tinref);
    
    ACE_OS::memcpy(&m_qinref, ptr, sizeof(m_qinref));
    ptr += sizeof(m_qinref);

#ifdef DEBUG
    assert(ptr == buf + BLKSZ);
#endif
}

FileNode::~FileNode()
{
    LOG(lgr, 6, "DTOR");
}

void
FileNode::bn_persist(Context & i_ctxt)
{
    uint8 buf[BlockNode::BLKSZ];

    // Clear the buffer first.  Seems unnecessary, but I don't
    // like leaking our stack contents out in blocks ...
    //
    ACE_OS::memset(buf, '\0', sizeof(buf));

    // Compute the size of fixed fields after the inode.
    size_t fixedsz = fixed_field_size();
    
    // Compute the size available for the inode fields and serialize.
    size_t sz = BlockNode::BLKSZ - sizeof(m_initvec) - fixedsz;
    bool rv = m_inode.SerializeToArray(buf + sizeof(m_initvec), sz);
    if (!rv)
        throwstream(InternalError, FILELINE << "inode serialization error");

    // Setup a pointer for the fixed fields.
    uint8 * ptr = &buf[BlockNode::BLKSZ - fixedsz];

    // Copy the inline data.
    ACE_OS::memcpy(ptr, m_inl, sizeof(m_inl));
    ptr += sizeof(m_inl);

    // Copy the block references.
    ACE_OS::memcpy(ptr, m_dirref, sizeof(m_dirref));
    ptr += sizeof(m_dirref);

    ACE_OS::memcpy(ptr, &m_sinref, sizeof(m_sinref));
    ptr += sizeof(m_sinref);

    ACE_OS::memcpy(ptr, &m_dinref, sizeof(m_dinref));
    ptr += sizeof(m_dinref);
    
    ACE_OS::memcpy(ptr, &m_tinref, sizeof(m_tinref));
    ptr += sizeof(m_tinref);
    
    ACE_OS::memcpy(ptr, &m_qinref, sizeof(m_qinref));
    ptr += sizeof(m_qinref);

#ifdef DEBUG
    assert(ptr == buf + BLKSZ);
#endif

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(m_initvec, 0, buf, sizeof(buf));

    // Write the initvec in the front.
    ACE_OS::memcpy(buf, m_initvec, sizeof(m_initvec));

    // Take the digest of the whole thing.
    bn_digest(Digest(buf, sizeof(buf)));

    LOG(lgr, 6, "persist " << bn_digest());

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(bn_digest().data(), bn_digest().size(),
                               buf, sizeof(buf));
}

void
FileNode::rb_traverse(Context & i_ctxt,
                      off_t i_base,
                      off_t i_rngoff,
                      size_t i_rngsize,
                      BlockTraverseFunc & i_trav)
{
    // Running offset.
    size_t off = 0;

    // Our base needs to be 0 or else somebody is very confused.
    if (i_base != off_t(off))
        throwstream(InternalError, FILELINE
                    << "FileNode::rb_traverse called with non-zero base: "
                    << i_base);

    RefBlockNode::BindingSeq mods;

    // Does this region intersect the inline block?
    if (i_rngoff < off_t(sizeof(m_inl)))
        if (i_trav.bt_visit(i_ctxt, m_inl, sizeof(m_inl), 0))
            mods.push_back(make_pair(off, Digest()));

    off += sizeof(m_inl);

    // Traverse the direct blocks.
    for (unsigned i = 0; i < NDIRECT; ++i)
    {
        // If we are beyond the traversal region we're done.
        if (off > i_rngoff + i_rngsize)
            break;

        // Does the range intersect this block?
        if (i_rngoff < off_t(off + BlockNode::BLKSZ))
        {
            // Do we have a cached version of this block?
            if (!m_dirobj[i])
            {
                // Nope, does it have a digest yet?
                if (m_dirref[i])
                {
                    // Yes, read it from the block store.
                    m_dirobj[i] = new DataBlockNode(i_ctxt, m_dirref[i]);
                }
                else
                {
                    // Nope, create new block.
                    m_dirobj[i] = new DataBlockNode();
                    m_dirobj[i]->bn_persist(i_ctxt);
                    m_dirref[i] = m_dirobj[i]->bn_digest();
                }
            }

            if (i_trav.bt_visit(i_ctxt,
                                m_dirobj[i]->bn_data(),
                                m_dirobj[i]->bn_size(),
                                off))
            {
                m_dirobj[i]->bn_persist(i_ctxt);
                mods.push_back(make_pair(off, m_dirobj[i]->bn_digest()));
            }
        }

        // Move to the next direct block.
        off += BlockNode::BLKSZ;
    }

    // Were there any modified regions?
    if (!mods.empty())
        i_trav.bt_update(i_ctxt, *this, mods);
}

void
FileNode::rb_update(Context & i_ctxt, BindingSeq const & i_bs)
{
    // FIXME - We need to store any modified bindings!
}

int
FileNode::getattr(Context & i_ctxt, struct stat * o_statbuf)
{
    ACE_OS::memset(o_statbuf, '\0', sizeof(*o_statbuf));

    o_statbuf->st_mode = m_inode.mode();
    o_statbuf->st_uid = 0;    // FIXME - uid missing!
    o_statbuf->st_gid = 0;    // FIXME - gid missing!
    o_statbuf->st_nlink = m_inode.nlink();
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
    if (i_off + i_size > sizeof(m_inl))
        throwstream(InternalError, FILELINE
                    << "FileNode::read outside inlined bytes unsupported");

    // Can't read past the end of data.
    if (i_off > off_t(size()))
        return 0;

    // Don't return bytes past the end of the file.
    size_t sz = min(i_size, size() - i_off);

    ACE_OS::memcpy(o_bufptr, m_inl + i_off, sz);
    return sz;
}

class WriteBTF : public RefBlockNode::BlockTraverseFunc
{
public:
    WriteBTF(void const * i_wrtdata,
             size_t i_wrtsize,
             off_t i_wrtoff)
        : m_wrtdata(i_wrtdata)
        , m_wrtsize(i_wrtsize)
        , m_wrtoff(i_wrtoff)
    {}

    //          wrtoff -------------------------------------> wrtsz
    // -----------+---------------------------------------------+------
    //            |                Write Buffer                 |
    // -----+-----+--------+------------------------+-----------+---+--
    //      |   Block A    |                        |    Block B    |
    // -----+--------------+------------------------+---------------+--
    //    blkoff ------> blksz                    blkoff -------> blksz


    virtual bool bt_visit(Context & i_ctxt,
                          void * i_blkdata,
                          size_t i_blksize,
                          off_t i_blkoff)
    {
        // Find the minimum file offset which is being transferred.
        off_t minoff = max(m_wrtoff, i_blkoff);

        // Find the file offset just after the write.
        off_t maxoff = min(m_wrtoff + m_wrtsize, i_blkoff + i_blksize);

        // If there is no overlap we are done.
        if (minoff >= maxoff)
            return false;

        // Find the source pointer.
        uint8 const * srcptr =
            ((uint8 const *) m_wrtdata) + (minoff - m_wrtoff);

        // Find the dest pointer.
        uint8 * dstptr = ((uint8 *) i_blkdata) + (minoff - i_blkoff);

        // Find the size of the transfer.
        size_t sz = maxoff - minoff;

        // Write the data.
        ACE_OS::memcpy(dstptr, srcptr, sz);

        // We wrote some bytes.
        m_retval += sz;
        return true;
    }

private:
    void const *	m_wrtdata;	// Data to write.
    size_t			m_wrtsize;	// Size of write.
    off_t			m_wrtoff;	// Logical offset in file
};

int
FileNode::write(Context & i_ctxt,
                void const * i_data,
                size_t i_size,
                off_t i_off)
{
    try
    {
        WriteBTF wbtf(i_data, i_size, i_off);
        rb_traverse(i_ctxt, 0, i_off, i_size, wbtf);

        // Did the node get bigger?
        // If the file grew the size needs to be larger.
        size(max(i_off + i_size, size()));

        // Save the new state to the blockstore.
        bn_persist(i_ctxt);

        return wbtf.bt_retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

size_t
FileNode::fixed_field_size() const
{
    return sizeof(m_inl)
        + sizeof(m_dirref)
        + sizeof(m_sinref)
        + sizeof(m_dinref)
        + sizeof(m_tinref)
        + sizeof(m_qinref);
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
