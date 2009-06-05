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
#include "BlockCipher.h"

#include "utfslog.h"

#include "BlockNode.h"
#include "BlockRef.h"
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

    m_inode.set_mode(S_IRUSR | S_IWUSR | S_IRGRP | S_IFREG);
    m_inode.set_nlink(1);
    m_inode.set_uname(myuname());
    m_inode.set_gname(mygname());
    m_inode.set_size(0);
    m_inode.set_atime(now.usec());
    m_inode.set_mtime(now.usec());
    m_inode.set_ctime(now.usec());
    m_inode.set_blocks(1);

    ACE_OS::memset(m_inl, '\0', sizeof(m_inl));
}

FileNode::FileNode(Context & i_ctxt, BlockRef const & i_ref)
    : RefBlockNode(i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);

    uint8 buf[BlockNode::BLKSZ];

    // Read the block from the blockstore.
    i_ctxt.m_bsh->bs_get_block(i_ref.data(), i_ref.size(),
                               buf, sizeof(buf));

    // Validate the block.
    i_ref.validate(buf, sizeof(buf));

    // Decrypt the block.
    i_ctxt.m_cipher.decrypt(i_ref.iv(), buf, sizeof(buf));

    // Compute the size of fixed fields after the inode.
    size_t fixedsz = fixed_field_size();

    // Parse the INode data.
    size_t sz = BlockNode::BLKSZ - fixedsz;
    
    // NOTE - We use protobuf components individually here
    // because ParseFromArray insists that the entire input
    // stream be consumed.
    //
    ArrayInputStream input(buf, sz);
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

BlockRef
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
    size_t sz = BlockNode::BLKSZ - fixedsz;
    bool rv = m_inode.SerializeToArray(buf, sz);
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

    uint8 iv[16];
    Random::fill(iv, sizeof(iv));

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(iv, buf, sizeof(buf));

    // Update our reference.
    m_ref = BlockRef(Digest(buf, sizeof(buf)), iv);

    LOG(lgr, 6, "persist " << m_ref);

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(m_ref.data(), m_ref.size(),
                               buf, sizeof(buf));

    return m_ref;
}

bool
FileNode::rb_traverse(Context & i_ctxt,
                      unsigned int i_flags,
                      off_t i_base,
                      off_t i_rngoff,
                      size_t i_rngsize,
                      BlockTraverseFunc & i_trav)
{
    // Running offset and size.
    off_t off = 0;
    off_t sz = 0;

    // Our base needs to be 0 or else somebody is very confused.
    if (i_base != off)
        throwstream(InternalError, FILELINE
                    << "FileNode::rb_traverse called with non-zero base: "
                    << i_base);

    RefBlockNode::BindingSeq mods;

    // Does this region intersect the inline block?
    //
    // NOTE - We don't really need to append to the mods collection
    // since this block is inline and the FileNode gets persisted
    // already.  But the return logic correctly will return true if we
    // use a placeholder.
    //
    if (i_rngoff < off_t(sizeof(m_inl)))
        if (i_trav.bt_visit(i_ctxt, m_inl, sizeof(m_inl), 0, size()))
            mods.push_back(make_pair(off, BlockRef()));

    off += sizeof(m_inl);

    // Are we beyond the region?
    if (off > i_rngoff + off_t(i_rngsize))
        goto done;

    // Does this region intersect the direct blocks?
    sz = NDIRECT * BlockNode::BLKSZ;
    if (i_rngoff >= off + sz)
    {
        // Skip past the direct blocks.
        off += sz;
    }
    else
    {
        // Traverse the direct blocks.
        for (unsigned i = 0; i < NDIRECT; ++i)
        {
            // If we are beyond the traversal region we're done.
            if (off > i_rngoff + off_t(i_rngsize))
                goto done;

            // Does the range intersect this block?
            if (i_rngoff < off_t(off + BlockNode::BLKSZ))
            {
                // Do we have a cached version of this block?
                if (!m_dirobj[i])
                {
                    // Nope, does it have a digest yet?
                    if (m_dirref[i])
                    {
                        // Yes, read it from the blockstore.
                        m_dirobj[i] = new DataBlockNode(i_ctxt, m_dirref[i]);
                    }
                    else if (i_flags & RB_MODIFY)
                    {
                        // Nope, create new block.
                        m_dirobj[i] = new DataBlockNode();

                        // Increment the block count.
                        m_inode.set_blocks(m_inode.blocks() + 1);
                    }
                    else
                    {
                        throwstream(InternalError, FILELINE
                                    << "read only traversal unimplemented");
                    }
                }

                if (i_trav.bt_visit(i_ctxt,
                                    m_dirobj[i]->bn_data(),
                                    m_dirobj[i]->bn_size(),
                                    off,
                                    size()))
                {
                    m_dirobj[i]->bn_persist(i_ctxt);
                    mods.push_back(make_pair(off, m_dirobj[i]->bn_blkref()));
                }
            }

            // Move to the next direct block.
            off += BlockNode::BLKSZ;
        }
    }

    // Are we beyond the region?
    if (off > i_rngoff + off_t(i_rngsize))
        goto done;

    // Does this region intersect the single indirect block?
    sz = IndirectBlockNode::NUMREF * BlockNode::BLKSZ;
    if (i_rngoff >= off + sz)
    {
        // Skip past the single indirect block ...
        off += sz;
    }
    else
    {
        // Do we have a cached version of this block?
        if (!m_sinobj)
        {
            // Nope, does it have a digest yet?
            if (m_sinref)
            {
                // Yes, read it from the blockstore.
                m_sinobj = new IndirectBlockNode(i_ctxt, m_sinref);
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create a new one.
                m_sinobj = new IndirectBlockNode();

                // Increment the block count.
                m_inode.set_blocks(m_inode.blocks() + 1);
            }
            else
            {
                throwstream(InternalError, FILELINE
                            << "read only traversal unimplemented");
            }
        }

        if (m_sinobj->rb_traverse(i_ctxt, i_flags, off,
                                  i_rngoff, i_rngsize, i_trav))
        {
            m_sinobj->bn_persist(i_ctxt);
            mods.push_back(make_pair(off, m_sinobj->bn_blkref()));
        }
    }

    // If we get here we need to use the indirect block ...
    throwstream(InternalError, FILELINE
                << "multiple indirect block traversal unimplemented");

 done:
    // Were there any modified regions?
    if (mods.empty())
    {
        return false;
    }
    else
    {
        i_trav.bt_update(i_ctxt, *this, mods);
        return true;
    }
}

void
FileNode::rb_update(Context & i_ctxt, BindingSeq const & i_bs)
{
    for (unsigned i = 0; i < i_bs.size(); ++i)
    {
        off_t off = i_bs[i].first;
        if (off == 0)
            continue; // Ignore, this is the inline section.

        // Subtract the inline block size.
        off -= sizeof(m_inl);

        if (off < off_t(NDIRECT * BLKSZ))
        {
            m_dirref[off/BLKSZ] = i_bs[i].second;
            continue;
        }

        off -= off_t(NDIRECT * BLKSZ);

        if (off == 0)
        {
            m_sinref = i_bs[i].second;
            continue;
        }

        off -= off_t(IndirectBlockNode::NUMREF * BlockNode::BLKSZ);

        // If we get here we need to implement more stuff ...
        throwstream(InternalError, FILELINE
                    << "FileNode::rb_update "
                    << "for multi indirect blocks unimplemented");
    }
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
    o_statbuf->st_blocks = m_inode.blocks() * 16;	// *= 8192 / 512

    return 0;
}

class ReadBTF : public RefBlockNode::BlockTraverseFunc
{
public:
    ReadBTF(void * i_rddata,
             size_t i_rdsize,
             off_t i_rdoff)
        : m_rddata(i_rddata)
        , m_rdsize(i_rdsize)
        , m_rdoff(i_rdoff)
    {}
    
    //          rdoff --------------------------------------> rdsz
    // -----------+---------------------------------------------+------
    //            |                Read Buffer                  |
    // -----+-----+--------+------------------------+-----------+---+--
    //      |   Block A    |                        |    Block B    |
    // -----+--------------+------------------------+---------------+--
    //    blkoff ------> blksz                    blkoff -------> blksz


    virtual bool bt_visit(Context & i_ctxt,
                          void * i_blkdata,
                          size_t i_blksize,
                          off_t i_blkoff,
                          size_t i_filesz)
    {
        // Find the minimum file offset which is being transferred.
        off_t minoff = max(m_rdoff, i_blkoff);

        // Find the file offset just after the write.
        off_t maxoff = min(m_rdoff + m_rdsize, i_blkoff + i_blksize);

        // Make sure we aren't reading past the end of the file.
        if (maxoff > off_t(i_filesz))
            maxoff = i_filesz;

        // If there is no overlap we are done.
        if (minoff >= maxoff)
            return false;

        // Find the dest pointer.
        uint8 * dstptr = ((uint8 *) m_rddata) + (minoff - m_rdoff);

        // Find the source pointer.
        uint8 const * srcptr =
            ((uint8 const *) i_blkdata) + (minoff - i_blkoff);

        // Find the size of the transfer.
        size_t sz = maxoff - minoff;

        // Write the data.
        ACE_OS::memcpy(dstptr, srcptr, sz);

        // We read some bytes.
        m_retval += sz;

        // But we didn't change any ...
        return false;
    }

private:
    void const *	m_rddata;	// Read data buffer.
    size_t			m_rdsize;	// Size of read.
    off_t			m_rdoff;	// Logical offset in file
};

int
FileNode::read(Context & i_ctxt, void * o_bufptr, size_t i_size, off_t i_off)
{
    try
    {
        ReadBTF rbtf(o_bufptr, i_size, i_off);
        rb_traverse(i_ctxt, RB_DEFAULT, 0, i_off, i_size, rbtf);
        return rbtf.bt_retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
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
                          off_t i_blkoff,
                          size_t i_filesz)
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
        rb_traverse(i_ctxt, RB_MODIFY, 0, i_off, i_size, wbtf);

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
