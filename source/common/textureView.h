#pragma once

#include <epoxy/gl.h>
#include "shaderProgram.h"
#include "shaders.h"
#include "openglValidation.h"

namespace at3 {

  class TextureView {

      GLuint vertices = 0;
      GLuint texCoords = 0;
      GLuint textureUnit = 0;

    public:
      TextureView();
      virtual ~TextureView() = default;
      void viewTextureUnit(GLuint unit);
      virtual void draw();
  };
}
