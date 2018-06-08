
#pragma once

#include <cassert>
#include <epoxy/gl.h>

#ifdef __EMSCRIPTEN__
#define DISABLE_CHECK_GL_ERROR true
#else
#define DISABLE_CHECK_GL_ERROR false
#endif

#define FORCE_CHECK_GL_ERROR() \
  (checkGlError(__FILE__, __LINE__))

#define FORCE_ASSERT_GL_ERROR() \
  if (checkGlError(__FILE__, __LINE__)) \
    assert(false);

#if DISABLE_CHECK_GL_ERROR

#define CHECK_GL_ERROR() (false)

#define ASSERT_GL_ERROR() ;

#else  // DISABLE_CHECK_GL_ERROR

#define CHECK_GL_ERROR() \
  (checkGlError(__FILE__, __LINE__))

#define ASSERT_GL_ERROR() \
  if (checkGlError(__FILE__, __LINE__)) \
    assert(false);

#define DISCARD_GL_ERROR() checkGlError(__FILE__, __LINE__)

#endif  // DISABLE_CHECK_GL_ERROR

namespace at3 {

  bool checkGlError(const char *file, int line);

  const char *glErrorToString(GLenum error); // FIXME: Rename this function?

  enum LoadResult {
    LOAD_SUCCESS, LOAD_NOT_FOUND, LOAD_NOT_POW_2
  };
}
