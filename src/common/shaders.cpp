/*
 * Copyright (c) 2016 Jonathan Glines, Galen Cochrane
 * Jonathan Glines <jonathan@glines.net>
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "shaderProgram.h"
#include "shaders.h"

namespace at3 {
  #define SHADER_DIR assets_shaders
  
  #define CAT(a, b) a ## b
  
  #define DEFINE_SHADER_BASE(shader, dir) \
  std::shared_ptr<ShaderProgram> Shaders:: shader ## Shader() { \
    static std::shared_ptr<ShaderProgram> instance = \
      std::shared_ptr<ShaderProgram>( \
          new ShaderProgram( \
            (const char *)CAT(dir, _ ## shader ## _vert), \
            CAT(dir, _ ## shader ## _vert_len), \
            (const char *)CAT(dir, _ ## shader ## _frag), \
            CAT(dir, _ ## shader ## _frag_len) \
            )); \
    return instance; \
  }
  #define DEFINE_SHADER_BASE_TESS(shader, dir) \
    std::shared_ptr<ShaderProgram> Shaders:: shader ## Shader() { \
      static std::shared_ptr<ShaderProgram> instance = \
        std::shared_ptr<ShaderProgram>( \
          new ShaderProgram( \
            (const char *)CAT(dir, _ ## shader ## _vert), \
            CAT(dir, _ ## shader ## _vert_len), \
            (const char *)CAT(dir, _ ## shader ## _frag), \
            CAT(dir, _ ## shader ## _frag_len), \
            (const char *)CAT(dir, _ ## shader ## _tesc), \
            CAT(dir, _ ## shader ## _tesc_len), \
            (const char *)CAT(dir, _ ## shader ## _tese), \
            CAT(dir, _ ## shader ## _tese_len) \
            )); \
    return instance; \
  }
  #define DEFINE_SHADER(shader) DEFINE_SHADER_BASE(shader, SHADER_DIR)
  #define DEFINE_SHADER_TESS(shader) DEFINE_SHADER_BASE_TESS(shader, SHADER_DIR)
  
  #include "assets_shaders_billboard.vert.c"
  #include "assets_shaders_billboard.frag.c"
  DEFINE_SHADER(billboard)
  
  #include "assets_shaders_billboardPoint.vert.c"
  #include "assets_shaders_billboardPoint.frag.c"
  DEFINE_SHADER(billboardPoint)
  
  #include "assets_shaders_gouraud.vert.c"
  #include "assets_shaders_gouraud.frag.c"
  DEFINE_SHADER(gouraud)
  
  #include "assets_shaders_wireframe.vert.c"
  #include "assets_shaders_wireframe.frag.c"
  DEFINE_SHADER(wireframe)
  
  #include "assets_shaders_texture.vert.c"
  #include "assets_shaders_texture.frag.c"
  DEFINE_SHADER(texture)
  
  #include "assets_shaders_skyQuad.vert.c"
  #include "assets_shaders_skyQuad.frag.c"
  DEFINE_SHADER(skyQuad)
  
  #include "assets_shaders_terrain.vert.c"
  #include "assets_shaders_terrain.tesc.c"
  #include "assets_shaders_terrain.tese.c"
  #include "assets_shaders_terrain.frag.c"
  DEFINE_SHADER_TESS(terrain)
}
