#pragma once

#include <memory>

namespace at3 {
  class ShaderProgram;
  /**
   * Contains static declarations for GL shaders that are used in various
   * sample programs. The GLSL source code for these shaders can be found in
   * the src/samples/common/assets/shaders directory.
   *
   * These shaders used to be read from files, but now the GLSL source code is
   * compiled into a C string within the samples_common static library. While
   * this is somewhat less flexible, it allows many of the sample programs to
   * run without any runtime file dependencies.
   */
  class Shaders {
    public:
#define DECLARE_SHADER(shader) static std::shared_ptr<ShaderProgram> shader ## Shader()

    /** Shader for drawing quad billboards. */
    DECLARE_SHADER(billboard);
    /** Shader for drawing points as billboards (since WebGL doesn't support
     * point sprites). */
    DECLARE_SHADER(billboardPoint);

    /** Shader implementing Gouraud shading. */
    DECLARE_SHADER(gouraud);

    /** Shader for drawing debugging lines. */
    DECLARE_SHADER(wireframe);

    /** Shader for drawing textures in full-bright lighting */
    DECLARE_SHADER(textureFullbright);

    /** Shader for drawing textures in sunny-looking weather */
    DECLARE_SHADER(textureSunny);

    /** Shader for drawing meshes without textures */
    DECLARE_SHADER(noTextureSunny);

    /** Shader for drawing skybox */
    DECLARE_SHADER(skyQuad);
    
    /** shader for drawing LOD terrain */
    DECLARE_SHADER(terrain);
  };
}
