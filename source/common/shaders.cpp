
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
