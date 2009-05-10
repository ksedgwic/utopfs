#ifndef UTFS_utfsexp_h__
#define UTFS_utfsexp_h__

#if defined(UTFS_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define UTFS_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define UTFS_EXP __attribute__ ((visibility("default")))
#       else
#           define UTFS_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define UTFS_EXP _declspec(dllimport)
#   else
#       define UTFS_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_utfsexp_h__
