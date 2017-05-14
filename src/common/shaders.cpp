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
# define SHADER_DIR shaders
  
# define CAT(a, b) a ## b
  
# define DEFINE_SHADER_BASE(shader, dir) \
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
# define DEFINE_SHADER_BASE_TESS(shader, dir) \
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
            CAT(dir, _ ## shader ## _tese_len), \
            (const char *)CAT(dir, _ ## shader ## _geom), \
            CAT(dir, _ ## shader ## _geom_len) \
            )); \
    return instance; \
  }
# define DEFINE_SHADER(shader) DEFINE_SHADER_BASE(shader, SHADER_DIR)
# define DEFINE_SHADER_TESS(shader) DEFINE_SHADER_BASE_TESS(shader, SHADER_DIR)
  
# include "shaders_billboard.vert.c"
# include "shaders_billboard.frag.c"
  DEFINE_SHADER(billboard)
  
# include "shaders_billboardPoint.vert.c"
# include "shaders_billboardPoint.frag.c"
  DEFINE_SHADER(billboardPoint)
  
# include "shaders_gouraud.vert.c"
# include "shaders_gouraud.frag.c"
  DEFINE_SHADER(gouraud)
  
# include "shaders_wireframe.vert.c"
# include "shaders_wireframe.frag.c"
  DEFINE_SHADER(wireframe)

# include "shaders_textureFullbright.vert.c"
# include "shaders_textureFullbright.frag.c"
  DEFINE_SHADER(textureFullbright)

# include "shaders_textureSunny.vert.c"
# include "shaders_textureSunny.frag.c"
  DEFINE_SHADER(textureSunny)

# include "shaders_noTextureSunny.vert.c"
# include "shaders_noTextureSunny.frag.c"
  DEFINE_SHADER(noTextureSunny)
  
# include "shaders_skyQuad.vert.c"
# include "shaders_skyQuad.frag.c"
  DEFINE_SHADER(skyQuad)
  
# include "shaders_terrain.vert.c"
# include "shaders_terrain.tesc.c"
# include "shaders_terrain.tese.c"
# include "shaders_terrain.frag.c"
# include "shaders_terrain.geom.c"
  DEFINE_SHADER_TESS(terrain)
}
