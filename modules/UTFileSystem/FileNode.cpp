#include <sys/types.h>

#if !defined(WIN32)
#include <pwd.h>
#include <grp.h>
#endif

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
#include "UTFileSystem.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {

// FIXME - Can these results be cached?

#if !defined(WIN32)
uid_t mapuname(string const & i_uname)
{
    struct passwd pwbuf;
    struct passwd * pwp;
    char strbuf[8192];

    getpwnam_r(i_uname.c_str(), &pwbuf, strbuf, sizeof(strbuf), &pwp);

    return pwp ? pwp->pw_uid : 0;
}

uid_t mapgname(string const & i_gname)
{
    struct group gbuf;
    struct group * gp;
    char strbuf[8192];

    getgrnam_r(i_gname.c_str(), &gbuf, strbuf, sizeof(strbuf), &gp);

    return gp ? gp->gr_gid : 0;
}
#endif
} // end namespace

namespace UTFS {

FileNode::FileNode(mode_t i_mode,
                   string const & i_uname,
                   string const & i_gname)
{
    LOG(lgr, 6, "CTOR");

    T64 now = T64::now();

    m_inode.set_mode(i_mode | S_IFREG);
    m_inode.set_nlink(1);
    m_inode.set_uname(i_uname);
    m_inode.set_gname(i_gname);
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
    size_t bsz = sizeof(buf);

    ACE_OS::memset(buf, '\0', bsz);

    // Read the block from the blockstore.
    i_ctxt.m_bsh->bs_block_get(i_ref.data(), i_ref.size(),
                               buf, bsz);

    ++i_ctxt.m_statsp->m_ngops;
    i_ctxt.m_statsp->m_ngbytes += bsz;

    // Validate the block.
    i_ref.validate(buf, bsz);

    // Decrypt the block.
    i_ctxt.m_cipher.decrypt(i_ref.iv(), buf, bsz);

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
    uint8 * ptr = &buf[sz];

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
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

BlockRef const &
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
    i_ctxt.m_bsh->bs_block_put((void *) m_ref.data(),
                               m_ref.size(),
                               (void *) buf,
                               sizeof(buf));

    ++i_ctxt.m_statsp->m_npops;
    i_ctxt.m_statsp->m_npbytes += sizeof(buf);

    bn_isdirty(false);

    return m_ref;
}

BlockRef const &
FileNode::bn_flush(Context & i_ctxt)
{
    // If we aren't dirty then we just return our current reference.
    if (!bn_isdirty())
        return bn_blkref();

    for (unsigned i = 0; i < NDIRECT; ++i)
    {
        if (m_dirobj_X[i])
        {
            // Flush the object.
            m_dirref[i] = m_dirobj_X[i]->bn_flush(i_ctxt);

            // Insert it in the clean cache.
            i_ctxt.m_bncachep->insert(m_dirobj_X[i]);

            // Clear it in the dirty array.
            m_dirobj_X[i] = NULL;
        }
    }

    if (m_sinobj_X)
    {
        m_sinref = m_sinobj_X->bn_flush(i_ctxt);
        i_ctxt.m_bncachep->insert(m_sinobj_X);
        m_sinobj_X = NULL;
    }

    if (m_dinobj_X)
    {
        m_dinref = m_dinobj_X->bn_flush(i_ctxt);
        i_ctxt.m_bncachep->insert(m_dinobj_X);
        m_dinobj_X = NULL;
    }

    if (m_tinobj_X)
    {
        m_tinref = m_tinobj_X->bn_flush(i_ctxt);
        i_ctxt.m_bncachep->insert(m_tinobj_X);
        m_tinobj_X = NULL;
    }

#if 0
    if (m_qinobj_X)
    {
        m_qinref = m_qinobj_X->bn_flush(i_ctxt);
        i_ctxt.m_bncachep->insert(m_qinobj_X);
        m_qinobj_X = NULL;
    }
#endif

    return bn_persist(i_ctxt);
}

bool
FileNode::rb_traverse(Context & i_ctxt,
                      FileNode & i_fn,
                      unsigned int i_flags,
                      off_t i_base,
                      off_t i_rngoff,
                      size_t i_rngsize,
                      BlockTraverseFunc & i_trav)
{
    // Running offset and size.
    off_t off = 0;
    off_t sz = 0;

    // ----------------------------------------------------------------
    // Inline Block
    // ----------------------------------------------------------------

    // Our base needs to be 0 or else somebody is very confused.
    if (i_base != off)
        throwstream(InternalError, FILELINE
                    << "FileNode::rb_traverse called with non-zero base: "
                    << i_base);

    // Does this region intersect the inline block?
    //
    // NOTE - We don't really need to append to the mods collection
    // since this block is inline and the FileNode gets persisted
    // already.  But the return logic correctly will return true if we
    // use a placeholder.
    //
    if (i_rngoff < off_t(sizeof(m_inl)))
    {
        if (i_trav.bt_visit(i_ctxt, m_inl, sizeof(m_inl), 0, size()))
        {
            bn_isdirty(true);
        }
    }

    off += sizeof(m_inl);

    // Are we beyond the region?
    if (off >= i_rngoff + off_t(i_rngsize))
        goto done;

    // ----------------------------------------------------------------
    // Direct References
    // ----------------------------------------------------------------

    // Does this region intersect the direct blocks?
    sz = NDIRECT * BLKSZ;
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
            if (off >= i_rngoff + off_t(i_rngsize))
                goto done;

            // Does the range intersect this block?
            if (i_rngoff < off_t(off + BlockNode::BLKSZ))
            {
                // Find the block object to use.
                DataBlockNodeHandle dbh;

                // Do we have a cached dirty version of this block?
                if (m_dirobj_X[i])
                {
                    // Yep, use it.
                    dbh = m_dirobj_X[i];
                }
                else
                {
                    // Nope, does it have a digest yet?
                    if (m_dirref[i])
                    {
                        // Does the clean cache have it?
                        BlockNodeHandle bnh =
                            i_ctxt.m_bncachep->lookup(m_dirref[i]);
                        if (bnh)
                        {
                            // Yes, better be a DataBlockNode ...
                            dbh = dynamic_cast<DataBlockNode *>(&*bnh);
                        }
                        else
                        {
                            // Nope, read it from the blockstore.
                            dbh = new DataBlockNode(i_ctxt, m_dirref[i]);

                            // Insert it in the clean cache.
                            i_ctxt.m_bncachep->insert(dbh);
                        }
                    }
                    else if (i_flags & RB_MODIFY)
                    {
                        // Nope, create new block.
                        dbh = new DataBlockNode();

                        // Keep it in the cache.
                        m_dirobj_X[i] = dbh;

                        // Increment the block count.
                        i_fn.blocks(i_fn.blocks() + 1);
                    }
                    else
                    {
                        // Use the zero singleton.
                        dbh = i_ctxt.m_zdatobj;

                        // And *don't* keep it in the cache!
                    }
                }

                if (i_trav.bt_visit(i_ctxt,
                                    dbh->bn_data(),
                                    dbh->bn_size(),
                                    off,
                                    size()))
                {
                    // It's dirty now.
                    dbh->bn_isdirty(true);

                    // Remove it from the clean cache.
                    i_ctxt.m_bncachep->remove(dbh->bn_blkref());

                    // Insert it in the dirty collection.
                    m_dirobj_X[i] = dbh;

                    // We're dirty too.
                    bn_isdirty(true);
                }
            }

            // Move to the next direct block.
            off += BlockNode::BLKSZ;
        }
    }

    // Are we beyond the region?
    if (off >= i_rngoff + off_t(i_rngsize))
        goto done;

    // ----------------------------------------------------------------
    // Single Indirect Reference
    // ----------------------------------------------------------------

    // Does this region intersect the single indirect block?
    sz = IndirectBlockNode::NUMREF * BlockNode::BLKSZ;
    if (i_rngoff < off + sz)
    {
        // Find the block object to use.
        IndirectBlockNodeHandle ibh;

        // Do we have a cached version of this block?
        if (m_sinobj_X)
        {
            // Yup, use it.
            ibh = m_sinobj_X;
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_sinref)
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_sinref);
                if (bnh)
                {
                    // Yes, better be a IndirectBlockNode ...
                    ibh = dynamic_cast<IndirectBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    ibh = new IndirectBlockNode(i_ctxt, m_sinref);

                    // Insert it in the clean cache.
                    i_ctxt.m_bncachep->insert(ibh);
                }
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create a new one.
                ibh = new IndirectBlockNode();

                // Keep it in the cache.
                m_sinobj_X = ibh;

                // Increment the block count.
                m_inode.set_blocks(m_inode.blocks() + 1);
            }
            else
            {
                // Use the zero singleton.
                ibh = i_ctxt.m_zsinobj;

                // And *don't* keep it in the cache!
            }
        }

        if (ibh->rb_traverse(i_ctxt, *this, i_flags, off,
                              i_rngoff, i_rngsize, i_trav))
        {
            // The node is already marked dirty ...

            // Remove it from the clean cache.
            i_ctxt.m_bncachep->remove(ibh->bn_blkref());

            // Insert it in the dirty collection.
            m_sinobj_X = ibh;

            // We're dirty too.
            bn_isdirty(true);
        }
    }

    // On to the next block.
    off += sz;

    // Are we beyond the region?
    if (off >= i_rngoff + off_t(i_rngsize))
        goto done;

    // ----------------------------------------------------------------
    // Double Indirect Reference
    // ----------------------------------------------------------------

    // Does this region intersect the double indirect block?
    sz =
        IndirectBlockNode::NUMREF *
        IndirectBlockNode::NUMREF *
        BlockNode::BLKSZ;

    if (i_rngoff < off + sz)
    {
        // Find the block object to use.
        DoubleIndBlockNodeHandle nh;

        // Do we have a cached version of this block?
        if (m_dinobj_X)
        {
            // Yup, use it.
            nh = m_dinobj_X;
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_dinref)
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_dinref);
                if (bnh)
                {
                    // Yes, better be a DoubleIndBlockNode ...
                    nh = dynamic_cast<DoubleIndBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    nh = new DoubleIndBlockNode(i_ctxt, m_dinref);

                    // Insert it in the clean cache.
                    i_ctxt.m_bncachep->insert(nh);
                }
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create a new one.
                nh = new DoubleIndBlockNode();

                // Keep it in the cache.
                m_dinobj_X = nh;

                // Increment the block count.
                m_inode.set_blocks(m_inode.blocks() + 1);
            }
            else
            {
                // Use the zero dingleton.
                nh = i_ctxt.m_zdinobj;

                // And *don't* keep it in the cache!
            }
        }

        if (nh->rb_traverse(i_ctxt, *this, i_flags, off,
                             i_rngoff, i_rngsize, i_trav))
        {
            // The node is already marked dirty ...

            // Remove it from the clean cache.
            i_ctxt.m_bncachep->remove(nh->bn_blkref());

            // Insert it in the dirty collection.
            m_dinobj_X = nh;

            // We're dirty too.
            bn_isdirty(true);
        }
    }

    // On to the next block.
    off += sz;

    // Are we beyond the region?
    if (off >= i_rngoff + off_t(i_rngsize))
        goto done;

    // ----------------------------------------------------------------
    // Tripled Indirect Reference
    // ----------------------------------------------------------------

    // Does this region intersect the triple indirect block?
    sz =
        IndirectBlockNode::NUMREF *
        IndirectBlockNode::NUMREF *
        IndirectBlockNode::NUMREF *
        BlockNode::BLKSZ;

    if (i_rngoff < off + sz)
    {
        // Find the block object to use.
        TripleIndBlockNodeHandle nh;

        // Do we have a cached version of this block?
        if (m_tinobj_X)
        {
            // Yup, use it.
            nh = m_tinobj_X;
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_tinref)
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_tinref);
                if (bnh)
                {
                    // Yes, better be a TripleIndBlockNode ...
                    nh = dynamic_cast<TripleIndBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    nh = new TripleIndBlockNode(i_ctxt, m_tinref);

                    // Insert it in the clean cache.
                    i_ctxt.m_bncachep->insert(nh);
                }
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create a new one.
                nh = new TripleIndBlockNode();

                // Keep it in the cache.
                m_tinobj_X = nh;

                // Increment the block count.
                m_inode.set_blocks(m_inode.blocks() + 1);
            }
            else
            {
                // Use the zero tingleton.
                nh = i_ctxt.m_ztinobj;

                // And *don't* keep it in the cache!
            }
        }

        if (nh->rb_traverse(i_ctxt, *this, i_flags, off,
                             i_rngoff, i_rngsize, i_trav))
        {
            // The node is already marked dirty ...

            // Remove it from the clean cache.
            i_ctxt.m_bncachep->remove(nh->bn_blkref());

            // Insert it in the dirty collection.
            m_tinobj_X = nh;

            // We're dirty too.
            bn_isdirty(true);
        }
    }

    // On to the next block.
    off += sz;

    // Are we beyond the region?
    if (off >= i_rngoff + off_t(i_rngsize))
        goto done;


    // ----------------------------------------------------------------
    // Quad Indirect Reference
    // ----------------------------------------------------------------

    // If we get here we need to code more.
    throwstream(InternalError, FILELINE
                << "more indirect block traversal implementation required");

 done:
    // Return our dirty state.
    return bn_isdirty();
}

size_t
FileNode::rb_truncate(Context & i_ctxt,
                      off_t i_base,
                      off_t i_size)
{
    // NOTE - This traversal is similar to the rb_traverse traversal.
    //
    // The traversal has three parts:
    //
    // 1) Blocks that are completely inside the new size are
    //    counted for the nblocks return value.
    //
    // 2) The block which contains the boundary (if the size is
    //    not block aligned) is zeroed after the size.
    //
    // 3) All blocks greater then the new boundary are dereferenced.

    size_t nblocks = 1;		// Start w/ the inode itself.
    off_t off = 0;
    off_t sz = 0;

    // Does the truncation intersect the inline blocks?
    //
    sz = INLSZ;
    if (i_size < off + sz)
    {
        // Zero the portion after the truncation.
        ACE_OS::memset(m_inl + i_size, '\0', sizeof(m_inl) - i_size);
        off += INLSZ;

        bn_isdirty(true);
    }

    off += INLSZ;

    // ---------------- Direct Blocks ----------------

    for (unsigned ndx = 0; ndx < NDIRECT; ++ndx)
    {
        // Is this block prior to the truncation?
        if (off + BLKSZ <= i_size)
        {
            // Increment the block counter if there is a data
            // block.
            //
            if (m_dirobj_X[ndx] || m_dirref[ndx])
                ++nblocks;
        }

        // Is the truncation inside this block?
        else if (off < i_size)
        {
            DataBlockNodeHandle dbh;

            // Do we have a cached version of this block?
            if (m_dirobj_X[ndx])
            {
                // Yep, use it.
                dbh = m_dirobj_X[ndx];
            }
            else
            {
                // Nope, does it have a digest yet?
                if (m_dirref[ndx])
                {
                    // Does the clean cache have it?
                    BlockNodeHandle bnh =
                        i_ctxt.m_bncachep->lookup(m_dirref[ndx]);
                    if (bnh)
                    {
                        // Yes, better be a DataBlockNode ...
                        dbh = dynamic_cast<DataBlockNode *>(&*bnh);
                    }
                    else
                    {
                        // Nope, read it from the blockstore.
                        dbh = new DataBlockNode(i_ctxt, m_dirref[ndx]);

                        // Insert it in the clean cache.
                        i_ctxt.m_bncachep->insert(dbh);
                    }
                }
                else
                {
                    // We don't have to create a new block
                    // since we are just going to zero part
                    // of it ...
                }
            }

            if (dbh)
            {
                // There is a block involved.
                ++nblocks;

                // Zero the data after the truncation.
                off_t off0 = i_size - off;
                ACE_OS::memset(dbh->bn_data() + off0,
                               '\0', 
                               dbh->bn_size() - off0);

#if 0
                // Seems we could just set the dirty flag instead?
                m_dirref[ndx] = dbh->bn_persist(i_ctxt);
#endif

                // It's dirty now.
                dbh->bn_isdirty(true);

                // Remove it from the clean cache.
                i_ctxt.m_bncachep->remove(dbh->bn_blkref());

                // Insert it in the dirty collection.
                m_dirobj_X[ndx] = dbh;

                // We're dirty too.
                bn_isdirty(true);
            }
        }

        // This block is after the truncation.
        else
        {
            // Remove from the clean cache.
            i_ctxt.m_bncachep->remove(m_dirref[ndx]);

            // We're just removing the references.
            m_dirref[ndx].clear();
            m_dirobj_X[ndx] = NULL;

            // We're dirty now.
            bn_isdirty(true);
        }

        off += BLKSZ;
    }

    // ---------------- Single Indirect ----------------

    sz = IndirectBlockNode::NUMREF * BLKSZ;

    // Is there an indirect block?
    if (m_sinobj_X || m_sinref)
    {
        // Does it overlap the live portion of the file?
        if (off < i_size)
        {
            IndirectBlockNodeHandle nh;

            // Is there a cached version?
            if (m_sinobj_X)
            {
                // Yes, just use it.
                nh = m_sinobj_X;
            }
            else
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_sinref);
                if (bnh)
                {
                    // Yes, better be a IndirectBlockNode ...
                    nh = dynamic_cast<IndirectBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    nh = new IndirectBlockNode(i_ctxt, m_sinref);

                    // Insert it in the clean cache.
                    i_ctxt.m_bncachep->insert(nh);
                }
            }

            // Traverse the indirect block.
            size_t nb = nh->rb_truncate(i_ctxt, off, i_size);

            if (nh->bn_isdirty())
            {
                // Remove it from the clean cache.
                i_ctxt.m_bncachep->remove(nh->bn_blkref());

                // Insert it in the dirty collection.
                m_sinobj_X = nh;

                // We're dirty too.
                bn_isdirty(true);
            }

            // Did it have child blocks?  If not purge it ...
            if (nb > 1)
            {
                // It had children, keep it.
                nblocks += nb;
            }
            else
            {
                // This seems redundant with the remove above, surely
                // one is enough?

                // Remove from the clean cache.
                i_ctxt.m_bncachep->remove(m_sinref);

                m_sinref.clear();
                m_sinobj_X = NULL;

                bn_isdirty(true);
            }
        }
        else
        {
            // Nope, this is in the truncated portion of the file.

            // Remove from the clean cache.
            i_ctxt.m_bncachep->remove(m_sinref);

            m_sinref.clear();
            m_sinobj_X = NULL;

            bn_isdirty(true);
        }
    }

    off += sz;

    // FIXME - Are we missing the DoubleIndBlock (and Triple ...) traversal?

    return nblocks;
}

size_t
FileNode::rb_refresh(Context & i_ctxt, uint64 i_rid)
{
    size_t nblocks = 0;

    BlockStore::KeySeq keys;

    // Refresh our own reference.
    keys.push_back(bn_blkref());
    ++nblocks;

    for (unsigned i = 0; i < NDIRECT; ++i)
    {
        if (m_dirref[i])
        {
            keys.push_back(m_dirref[i]);
            ++nblocks;
        }
    }

    if (m_sinref)
    {
        keys.push_back(m_sinref);
        ++nblocks;

        IndirectBlockNodeHandle nh;
        if (m_sinobj_X)
        {
            nh = m_sinobj_X;
        }
        else
        {
            // Does the clean cache have it?
            BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_sinref);
            if (bnh)
            {
                // Yes, better be a IndirectBlockNode ...
                nh = dynamic_cast<IndirectBlockNode *>(&*bnh);
            }
            else
            {
                // Nope, read it from the blockstore.
                nh = new IndirectBlockNode(i_ctxt, m_sinref);

                // Let's not insert refresh blocks it in the clean
                // cache ...
            }
        }

        nblocks += nh->rb_refresh(i_ctxt, i_rid);
    }

    if (m_dinref)
    {
        keys.push_back(m_dinref);
        ++nblocks;

        DoubleIndBlockNodeHandle nh;
        if (m_dinobj_X)
        {
            nh = m_dinobj_X;
        }
        else
        {
            // Does the clean cache have it?
            BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_dinref);
            if (bnh)
            {
                // Yes, better be a DoubleIndBlockNode ...
                nh = dynamic_cast<DoubleIndBlockNode *>(&*bnh);
            }
            else
            {
                // Nope, read it from the blockstore.
                nh = new DoubleIndBlockNode(i_ctxt, m_dinref);

                // Let's not insert refresh blocks it in the clean
                // cache ...
            }
        }

        nblocks += nh->rb_refresh(i_ctxt, i_rid);
    }

    if (m_tinref)
    {
        keys.push_back(m_tinref);
        ++nblocks;

        TripleIndBlockNodeHandle nh;
        if (m_tinobj_X)
        {
            nh = m_tinobj_X;
        }
        else
        {
            // Does the clean cache have it?
            BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_tinref);
            if (bnh)
            {
                // Yes, better be a TripleIndBlockNode ...
                nh = dynamic_cast<TripleIndBlockNode *>(&*bnh);
            }
            else
            {
                // Nope, read it from the blockstore.
                nh = new TripleIndBlockNode(i_ctxt, m_tinref);

                // Let's not insert refresh blocks it in the clean
                // cache ...
            }
        }

        nblocks += nh->rb_refresh(i_ctxt, i_rid);
    }

#if 0
    if (m_qinref)
    {
        keys.push_back(m_qinref);
        ++nblocks;

        QuadIndBlockNodeHandle nh;
        if (m_qinobj_X)
        {
            nh = m_qinobj_X;
        }
        else
        {
            // Does the clean cache have it?
            BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_qinref);
            if (bnh)
            {
                // Yes, better be a QuadIndBlockNode ...
                nh = dynamic_cast<QuadIndBlockNode *>(&*bnh);
            }
            else
            {
                // Nope, read it from the blockstore.
                nh = new QuadIndBlockNode(i_ctxt, m_qinref);

                // Let's not insert refresh blocks it in the clean
                // cache ...
            }
        }

        nblocks += nh->rb_refresh(i_ctxt, i_rid);
    }
#endif

    BlockStore::KeySeq missing;
    i_ctxt.m_bsh->bs_refresh_blocks(i_rid, keys, missing);
    if (!missing.empty())
    {
        LOG(lgr, 1, "FileNode::rb_refresh: " << i_rid
            << " missing " << missing.size() << " blocks");
    }

    return nblocks;
}

int
FileNode::getattr(Context & i_ctxt, struct statstb * o_statbuf)
{
    ACE_OS::memset(o_statbuf, '\0', sizeof(*o_statbuf));

    

    o_statbuf->st_mode = m_inode.mode();
#if !defined (WIN32)
    o_statbuf->st_uid = mapuname(m_inode.uname());
    o_statbuf->st_gid = mapgname(m_inode.gname());
#endif
    o_statbuf->st_nlink = m_inode.nlink();
    o_statbuf->st_size = m_inode.size();
    o_statbuf->st_atime = m_inode.atime() / 1000000;
    o_statbuf->st_mtime = m_inode.mtime() / 1000000;
    o_statbuf->st_ctime = m_inode.ctime() / 1000000;
    o_statbuf->st_blocks = m_inode.blocks() * 16;	// *= 8192 / 512

    return 0;
}

int
FileNode::readlink(Context & i_ctxt,
                      char * o_obuf,
                      size_t i_size)
{
    // We are not a symbolic link!
    throw EINVAL;
}

int
FileNode::chmod(Context & i_ctxt, mode_t i_mode)
{
    // We only update the permissions bits.
    mode_t md = mode();
    md &= ~ALLPERMS;			// Remove existing permissions.
    md |= (i_mode & ALLPERMS);	// Add permissions from arg.
    mode(md);

    // Doesn't look like chmod sets the modifiction time.

    bn_isdirty(true);

    return 0;
}

int
FileNode::truncate(Context & i_ctxt, off_t i_size)
{
    // Traverse adjusting block references.
    size_t nblocks = rb_truncate(i_ctxt, 0, i_size);

    // Set the metadata.
    blocks(nblocks);
    size(i_size);

    mtime(T64::now());

    bn_isdirty(true);

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
        rb_traverse(i_ctxt, *this, RB_DEFAULT, 0, i_off, i_size, rbtf);
        return rbtf.bt_retval();
    }
    catch (NotFoundError const & ex)
    {
        // A block we need is missing, fail this read w/ an IO error.
        LOG(lgr, 1, "IO ERROR: " << ex.what());
        return -EIO;
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
        if (rb_traverse(i_ctxt, *this, RB_MODIFY, 0, i_off, i_size, wbtf))
        {
            // Did the node get bigger?
            // If the file grew the size needs to be larger.
            size(max(i_off + i_size, size()));

            bn_isdirty(true);
        }

        return wbtf.bt_retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

int
FileNode::access(Context & i_ctxt, mode_t i_mode)
{
    // FIXME - This routine is obviously broken.  But it needs
    // proper uid/gid support before we can do it correctly.
    //
    // I'm confused; how do we know the uid/gid of the user making
    // this call?
    //
    return 0;
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
