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

#ifndef LD2016_COMMON_SHADER_PROGRAM_H_
#define LD2016_COMMON_SHADER_PROGRAM_H_

#include <epoxy/gl.h>
#include <string>

namespace at3 {
  /**
   * Class representing a compiled and linked GL shader program, suitable for
   * rendering GL graphics.
   */
  class ShaderProgram {
    private:
      // FIXME: Clean these up with a couple macros
      // FIXME: Remove "Location" from all of these variables
      GLuint m_shaderProgram;
      GLint m_modelViewLocation, m_projectionLocation,
             m_modelViewProjectionLocation, m_normalTransformLocation,
             m_lightPositionLocation, m_lightIntensityLocation,
             m_timeLocation, m_colorLocation;
      GLint m_vertPositionLocation, m_vertNormalLocation, m_vertColorLocation,
             m_vertTexCoordLocation, m_vertVelocityLocation;
      GLint m_vertStartTimeLocation;
      GLint m_texture0, m_terrain;
      GLint m_screenSize, m_mvp, m_lodFidelity, m_debugLines, m_maxPatchSize, m_maxFieldViewDot;

      static GLuint m_compileShader(const char *code, int code_len, GLenum type);
      static void m_linkShaderProgram(GLuint shaderProgram);

      void m_init(const char *vert, unsigned int vert_len, const char *frag, unsigned int frag_len);
    
      void m_init(const char *vert, unsigned int vert_len, const char *frag, unsigned int frag_len,
                  const char *tesc, unsigned int tesc_len, const char *tese, unsigned int tese_len,
                  const char *geom, unsigned int geom_len);
      
      void m_readCode(const std::string &path, char **code, unsigned int *code_len);

    protected:
      /**
       * A virtual member function to allow derived classes the opportunity to
       * register attribute and/or uniform locations immediately after the shader
       * program has been linked.
       */
      // FIXME: This virtual call cannot be made in the constructor... I should
      //        just remove this.
      virtual void initLocations();

    public:
      /**
       * Constructs an OpenGL shader program compiled and linked from the given
       * vertex and fragment shaders.
       *
       * \param vert Path to the vertex shader to compile.
       * \param frag Path to the fragment shader to compile.
       */
      ShaderProgram(const std::string &vert, const std::string &frag);
      /**
       * Constructs an OpenGL shader program compiled and linked directly from
       * string given for the vertex and fragment shader source.
       *
       * \param vert Source code string of the vertex shader to compile.
       * \param vert_len Length of the vertex shader source code string.
       * \param frag Source code string of the fragment shader to compile.
       * \param frag_len Length of the fragment shader source code string.
       */
      ShaderProgram(const char *vert, unsigned int vert_len, const char *frag, unsigned int frag_len);
      /**
       * Constructs an OpenGL shader program compiled and linked from the given
       * vertex, fragment, tesselation control, and tesselation evaluation shaders.
       *
       * \param vert Path to the vertex shader to compile.
       * \param frag Path to the fragment shader to compile.
       * \param tesc Path to the tesselation control shader to compile.
       * \param tese Path to the tesselation evaluation shader to compile.
       * \param geom Path to the geometry shader to compile.
       */
      ShaderProgram(const std::string &vert, const std::string &frag, const std::string &tesc, const std::string &tese,
                    const std::string &geom);
      /**
       * Constructs an OpenGL shader program compiled and linked directly from
       * string given for the vertex and fragment shader source.
       *
       * \param vert Source code string of the vertex shader to compile.
       * \param vert_len Length of the vertex shader source code string.
       * \param frag Source code string of the fragment shader to compile.
       * \param frag_len Length of the fragment shader source code string.
       * \param tesc Source code string of the tesselation control shader to compile.
       * \param tesc_len Length of the tesselation control source code string.
       * \param tese Source code string of the tesselation evaluation shader to compile.
       * \param tese_len Length of the tesselation evaluation source code string.
       * \param geom Source code string of the geometry shader to compile.
       * \param geom_len Length of the geometry source code string.
       */
      ShaderProgram(const char *vert, unsigned int vert_len, const char *frag, unsigned int frag_len,
                    const char *tesc, unsigned int tesc_len, const char *tese, unsigned int tese_len,
                    const char *geom, unsigned int geom_len);
      /**
       * Destroy this shader program.
       *
       * \todo Free the GL resources associated with ShaderProgram upon
       * destruction.
       */
      virtual ~ShaderProgram();

      /**
       * \return Location of the model-view transform matrix uniform in the
       * shader.
       */
      GLint modelViewLocation() const { return m_modelViewLocation; }
      /**
       * \return Location of the projection transform matrix uniform in the
       * shader.
       */
      GLint projectionLocation() const { return m_projectionLocation; }
      /**
       * \return Location of the combination model-view-projection transform
       * matrix uniform in the shader.
       */
      GLint modelViewProjectionLocation() const { return m_modelViewProjectionLocation; }
      /**
       * \return Location of the normal transform matrix for transforming
       * surface normals into view space.
       *
       * \todo Does the normal transform go from model to view space? I don't
       * remember.
       */
      GLint normalTransformLocation() const { return m_normalTransformLocation; }
      /**
       * \return Location of the point light position uniform in the shader.
       *
       * \todo Replace single point light location with an array of structs.
       */
      GLint lightPositionLocation() const { return m_lightPositionLocation; }
      /**
       * \return Location of the point light intensity uniform in the shader.
       */
      GLint lightIntensityLocation() const { return m_lightIntensityLocation; }
      /**
       * \return Location of the current time uniform in the shader.
       */
      GLint timeLocation() const { return m_timeLocation; }
      /**
       * \return Location of the color uniform in the shader.
       */
      GLint colorLocation() const { return m_colorLocation; }
      /**
       * \return Location of the vertex position vertex attribute in the
       * shader.
       */
      GLint vertPositionLocation() const { return m_vertPositionLocation; }
      /**
       * \return Location of the vertex normal vertex attribute in the
       * shader.
       */
      GLint vertNormalLocation() const { return m_vertNormalLocation; }
      /**
       * \return Location of the vertex color vertex attribute in the shader.
       * This is used primarily for specifying the color for drawing debugging
       * points and lines.
       */
      GLint vertColorLocation() const { return m_vertColorLocation; }
      /**
       * \return Location of the texture coordinate vertex attribute in the
       * shader.
       */
      GLint vertTexCoordLocation() const { return m_vertTexCoordLocation; }
      /**
       * \return Location of the vertex velocity attribute in the shader.
       *
       * \todo This probably is not being used. Remove the vertVelocity
       * attribute from ShaderProgram.
       */
      GLint vertVelocityLocation() const { return m_vertVelocityLocation; }
      /**
       * \return Location of the start time uniform in the shader.
       *
       * \todo This probably is not being used. Remove the vertStartTime
       * attribute from ShaderProgram.
       */
      GLint vertStartTimeLocation() const { return m_vertStartTimeLocation; }
      /**
       * \return Location of the first texture sampler uniform in the shader.
       */
      GLint texture0() const { return m_texture0; }
      /**
       * \return Location of the first texture sampler uniform in the shader.
       */
      GLint terrain() const { return m_terrain; }
      /**
       * \return Location of the first texture sampler uniform in the shader.
       */
      GLint screenSize() const { return m_screenSize; }
      /**
       * \return Location of the first texture sampler uniform in the shader.
       */
      GLint mvp() const { return m_mvp; }
      /**
       * \return Location of the first texture sampler uniform in the shader.
       */
      GLint lodFidelity() const { return m_lodFidelity; }
      /**
       * \return Location of the debugLines boolean in the shader.
       */
      GLint debugLines() const { return m_debugLines; }
      /**
       * \return Location of the maxPatchSize tesellation parameter in the shader.
       */
      GLint maxPatchSize() const { return m_maxPatchSize; }
      /**
       * \return Location of the maxFieldViewDot tesellation parameter in the shader.
       */
      GLint maxFieldViewDot() const { return m_maxFieldViewDot; }

      /**
       * Use this shader in the current GL state.
       */
      void use() const;
  };
}

#endif
