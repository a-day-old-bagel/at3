
#pragma once

#include "loadCubeMap.h"
#include "loadedTexture.h"
#include "shaders.h"
#include "shaderProgram.h"

namespace at3 {
  class Material {
      union {
        std::shared_ptr<ShaderProgram> mpShaderGlsl;
      };
  };
}
