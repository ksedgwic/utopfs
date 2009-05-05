#ifndef DFLG_dflgexp_h__
#define DFLG_dflgexp_h__

#if defined(DFLG_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define DFLG_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define DFLG_EXP __attribute__ ((visibility("default")))
#       else
#           define DFLG_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define DFLG_EXP _declspec(dllimport)
#   else
#       define DFLG_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLG_cmnexp_h__
