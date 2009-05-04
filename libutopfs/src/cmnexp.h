#ifndef cmn_cmnexp_h__
#define cmn_cmnexp_h__

#if defined(CMN_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define CMN_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define CMN_EXP __attribute__ ((visibility("default")))
#       else
#           define CMN_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define CMN_EXP _declspec(dllimport)
#   else
#       define CMN_EXP
#   endif
#endif

// With G++ exceptions must always be exported in *all* modules
// See http://gcc.gnu.org/wiki/Visibility for more info.
//
#if defined(WIN32)
#   define CMN_EXC_EXP CMN_EXP
#else
#   if defined(GCC_HASCLASSVISIBILITY)
#      define CMN_EXC_EXP __attribute__ ((visibility("default")))
#   else
#      define CMN_EXC_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // cmn_cmnexp_h__
