#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define VKBB_FANCY_WIN32_DEBUG
#endif

/*
 * NDEBUG is more portable than Visual Studio's _DEBUG
 */
#ifdef NDEBUG // Release build
# define checkf(expr, format, ...) ;
#else // Debug Build
# ifdef VKBB_FANCY_WIN32_DEBUG
#   include <atlstr.h>
#   define checkf(expr, format, ...) if (!(expr))																                            \
    {																											                                                  \
      fprintf(stdout, "CHECK FAILED: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);	\
      CString dbgstr;																							                                          \
      dbgstr.Format(("%s"), format);																			                                  \
      MessageBox(NULL,dbgstr, "FATAL ERROR", MB_OK);															                          \
    DebugBreak();																							                                              \
    }
# else
#   include <csignal>
#   include <cstdio>
#   define checkf(expr, format, ...) if (!(expr))                                                           \
    {                                                                                                       \
      fprintf(stdout, "CHECK FAILED: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);  \
      raise(SIGTRAP);                                                                                       \
    }
#   ifndef _DEBUG
#     define _DEBUG true // Since the rest of the code uses this VS define, lets define it for non-VS users
#   endif
# endif
#endif
