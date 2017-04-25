//
// Created by volundr on 4/24/2017.
//

#ifndef AT3_LOADEDTEXTURE_H
#define AT3_LOADEDTEXTURE_H

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


#endif //AT3_LOADEDTEXTURE_H
