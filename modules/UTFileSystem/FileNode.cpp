#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

#include "Log.h"
#include "Except.h"

#include "Random.h"

#include "utfslog.h"

#include "FileNode.h"

using namespace std;
using namespace utp;

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

    m_inode.set_mode(S_IRUSR | S_IWUSR | S_IRGRP);
    m_inode.set_uname(myuname());
    m_inode.set_gname(mygname());
    m_inode.set_size(0);
    m_inode.set_atime(now.usec());
    m_inode.set_mtime(now.usec());
    m_inode.set_ctime(now.usec());
}

FileNode::~FileNode()
{
    LOG(lgr, 4, "DTOR");
}

void
FileNode::persist()
{
    throwstream(InternalError, FILELINE
                << "FileNode::persist unimplemented");
}

int
FileNode::getattr(struct stat * o_statbuf)
{
    throwstream(InternalError, FILELINE
                << "FileNode::getattr unimplemented");
}

int
FileNode::read(void * o_bufptr, size_t i_size, off_t i_off)
{
    throwstream(InternalError, FILELINE
                << "FileNode::read unimplemented");
}

int
FileNode::write(void const * i_data, size_t i_size, off_t i_off)
{
    throwstream(InternalError, FILELINE
                << "FileNode::write unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
