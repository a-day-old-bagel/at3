//
// Created by volundr on 4/24/2017.
//

#include <stb_image.h>
#include "loadedTexture.h"
#include "glError.h"

namespace at3 {

  void at3::LoadedTexture::m_loadTexture(const std::string &textureFile) {
    if (textureFile.empty()) {
      return;
    }
    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(textureFile.c_str(), &x, &y, &n, 0);
    if (data) {
      // Create the texture object in the GL
      glGenTextures(1, &m_texture);
      FORCE_ASSERT_GL_ERROR();
      glBindTexture(GL_TEXTURE_2D, m_texture);
      FORCE_ASSERT_GL_ERROR();
      // Copy the image to the GL
      glTexImage2D(
          GL_TEXTURE_2D,  // target
          0,  // level
          n == 3 ? GL_RGB : GL_RGBA,  // internal format
          x,  // width
          y,  // height
          0,  // border
		      n == 3 ? GL_RGB : GL_RGBA,  // format
          GL_UNSIGNED_BYTE,  // type
          data  // data
      );
      FORCE_ASSERT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      FORCE_ASSERT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      FORCE_ASSERT_GL_ERROR();

      glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);         FORCE_ASSERT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                       FORCE_ASSERT_GL_ERROR();
    } else {
      fprintf(stderr, "Failed to load texture file '%s'.\n", textureFile.c_str());
    }
    stbi_image_free(data);
  }

  LoadedTexture::LoadedTexture() { }

  LoadedTexture::LoadedTexture(const std::string &textureFile) {
    m_loadTexture(textureFile);
  }

  LoadedTexture::~LoadedTexture() {

  }

  GLuint LoadedTexture::get() {
    return m_texture;
  }
}
