#pragma once

#include <epoxy/gl.h>
#include <string>

namespace at3 {
  class LoadedTexture {

    public:
      enum RequestedType {
        SINGLE, CUBE
      };
      enum LoadedType {
        INVALID, SINGLE_RGB, SINGLE_RGBA, CUBE_RGB
      };
      LoadedTexture(const std::string &textureFile, RequestedType requestedType);
      virtual ~LoadedTexture();
      GLuint get();

    private:
      GLuint mTexture = 0;
      LoadedType mStatus = INVALID;

      void loadSingleTexture(const std::string &textureFile);
      void loadCubeTexture(const std::string &textureFileBaseName);
      int loadCubeSide(GLenum side_target, const std::string file_name);
  };
}
