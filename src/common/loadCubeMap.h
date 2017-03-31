/* 
 * File:   loadCubeMap.h
 * Author: adayoldbagel
 *
 */

#include <GL/glew.h>

#ifndef LOADCUBEMAP_H
#define  LOADCUBEMAP_H

namespace ld2016 {

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

#endif	/* LOADCUBEMAP_H */

