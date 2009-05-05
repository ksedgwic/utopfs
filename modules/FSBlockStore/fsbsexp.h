#ifndef FSBS_fsbsexp_h__
#define FSBS_fsbsexp_h__

#if defined(FSBS_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define FSBS_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define FSBS_EXP __attribute__ ((visibility("default")))
#       else
#           define FSBS_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define FSBS_EXP _declspec(dllimport)
#   else
#       define FSBS_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // FSBS_fsbsexp_h__
