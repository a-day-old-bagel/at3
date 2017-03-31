
#include "loadCubeMap.h"
#include "stb_image.h"

namespace ld2016 {

  LoadResult readAndBufferCubeMap(const char *front,
                                  const char *back,
                                  const char *top,
                                  const char *bottom,
                                  const char *left,
                                  const char *right,
                                  GLuint *tex_cube) {
    LoadResult r;
    #define CONTINUE_IF_SUCCESS(t, s, f) r = loadCubeMapSide(t, s, f); if (r != LOAD_SUCCESS) return r;
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front);
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back);
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top);
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom);
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left);
    CONTINUE_IF_SUCCESS(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right);
    return LOAD_SUCCESS;
  }

  LoadResult loadCubeMapSide(GLuint texture, GLenum side_target, const char *file_name) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    int x, y, n;
    int force_channels = 4;
    unsigned char *image_data = stbi_load(file_name, &x, &y, &n, force_channels);
    if (!image_data) {
      return LOAD_NOT_FOUND;
    }
    // non-power-of-2 dimensions check
    if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
      return LOAD_NOT_POW_2;
    }
    // copy image data into 'target' side of cube map
    glTexImage2D(side_target, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return LOAD_SUCCESS;
  }
}
