
#pragma once

#include "loadedTexture.h"
#include "shaders.h"
#include "shaderProgram.h"

namespace at3 {
  struct RenderMethodInfo {
    enum Style {
      LINES, INDEXED_LINES, TRIANGLES, INDEXED_TRIANGLES
    };
    Style mStyle;
  };
  class Material {
      union {
        std::shared_ptr<ShaderProgram> mpShaderGlsl;
      };
      RenderMethodInfo mRenderMethodInfo;
  };
}
