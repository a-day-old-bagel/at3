#pragma once

#include <epoxy/gl.h>

namespace at3 {

  enum LoadResult {
    LOAD_SUCCESS, LOAD_NOT_FOUND, LOAD_NOT_POW_2
  };

  LoadResult readAndBufferCubeMap(const char *front,
                                  const char *back,
                                  const char *top,
                                  const char *bottom,
                                  const char *left,
                                  const char *right,
                                  GLuint *tex_cube);

  LoadResult loadCubeMapSide(GLuint texture, GLenum side_target, const char *file_name);

}
