
#pragma once

/*
 * Defines a custom assert-like macro that opens a message box on Windows.
 */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
# define AT3_FANCY_WIN32_DEBUG
#endif
#ifdef NDEBUG // Release build
# define AT3_ASSERT(expr, format, ...) ;
#else // Debug Build
# ifdef AT3_FANCY_WIN32_DEBUG
#   include <atlstr.h>
#   define AT3_ASSERT(expr, format, ...) if (!(expr))																                        \
    {																											                                                  \
      fprintf(stdout, "FAIL: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);	\
      CString dbgstr;																							                                          \
      dbgstr.Format(("%s"), format);																			                                  \
      MessageBox(NULL,dbgstr, "FATAL ERROR", MB_OK);															                          \
      DebugBreak();																						                                              \
    }
# else
#   include <csignal>
#   include <cstdio>
#   define AT3_ASSERT(expr, format, ...) if (!(expr))                                                       \
    {                                                                                                       \
      fprintf(stderr, "FAIL: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);  \
      raise(SIGTRAP);                                                                                       \
    }
# endif
#endif

// Aligned data
#if defined(_MSC_VER)
# define AT3_ALIGNED_(x) __declspec(align(x))
#else
# if defined(__GNUC__)
#   define AT3_ALIGNED_(x) __attribute__ ((aligned(x)))
# endif
#endif
