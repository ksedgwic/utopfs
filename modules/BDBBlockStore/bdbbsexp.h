#ifndef BDBBS_bdbbsexp_h__
#define BDBBS_bdbbsexp_h__

#if defined(BDBBS_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define BDBBS_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define BDBBS_EXP __attribute__ ((visibility("default")))
#       else
#           define BDBBS_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define BDBBS_EXP _declspec(dllimport)
#   else
#       define BDBBS_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // BDBBS_bdbbsexp_h__
