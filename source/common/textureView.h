#pragma once

#include <epoxy/gl.h>
#include "shaderProgram.h"
#include "shaders.h"
#include "openglValidation.h"

namespace at3 {

  class TextureView {

      GLuint m_vertices = 0;
      GLuint m_texCoords = 0;
      GLuint m_textureUnit = 0;

    public:
      TextureView();
      virtual ~TextureView() = default;
      void viewTextureUnit(GLuint unit);
      virtual void draw();
  };
}
