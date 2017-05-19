#pragma once

#include <epoxy/gl.h>
#include <string>

namespace at3 {
  class LoadedTexture {
      GLuint m_texture;

      void m_loadTexture(const std::string &textureFile);

    public:
      LoadedTexture();
      LoadedTexture(const std::string &textureFile);
      virtual ~LoadedTexture();
      GLuint get();
  };
}
