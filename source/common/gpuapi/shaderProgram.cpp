
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <cstring>

#include "shaderProgram.h"

#define GLSL_VERSION "400"

namespace at3 {
  ShaderProgram::ShaderProgram(
      const std::string &vert, const std::string &frag)
  {
    char *vertCode, *fragCode;
    unsigned int vertCodeLen, fragCodeLen;

    // Read the vertex and fragment shaders from file
    m_readCode(vert, &vertCode, &vertCodeLen);
    m_readCode(frag, &fragCode, &fragCodeLen);

    // Compile and link our shader
    m_init(
        vertCode, vertCodeLen,
        fragCode, fragCodeLen);

    delete[] vertCode;
    delete[] fragCode;
  }

  ShaderProgram::ShaderProgram(
      const char *vert, unsigned int vert_len,
      const char *frag, unsigned int frag_len)
  {
    // Compile and link our shader
    m_init(vert, vert_len, frag, frag_len);
  }
  
  ShaderProgram::ShaderProgram(const std::string& vert, const std::string& frag, const std::string& tesc,
                               const std::string& tese, const std::string &geom) {
    char *vertCode, *fragCode, *tescCode, *teseCode, *geomCode;
    unsigned int vertCodeLen, fragCodeLen, tescCodeLen, teseCodeLen, geomCodeLen;
  
    // Read the vertex and fragment shaders from file
    m_readCode(vert, &vertCode, &vertCodeLen);
    m_readCode(frag, &fragCode, &fragCodeLen);
    m_readCode(tesc, &tescCode, &tescCodeLen);
    m_readCode(tese, &teseCode, &teseCodeLen);
    m_readCode(geom, &geomCode, &geomCodeLen);

    // Compile and link our shader
    m_init(vertCode, vertCodeLen, fragCode, fragCodeLen,
           tescCode, tescCodeLen, teseCode, teseCodeLen, geomCode, geomCodeLen);
  
    delete[] vertCode;
    delete[] fragCode;
    delete[] tescCode;
    delete[] teseCode;
    delete[] geomCode;
  }
  
  ShaderProgram::ShaderProgram(const char* vert, unsigned int vert_len, const char* frag, unsigned int frag_len,
                               const char* tesc, unsigned int tesc_len, const char* tese, unsigned int tese_len,
                               const char *geom, unsigned int geom_len)
  {
    m_init(vert, vert_len, frag, frag_len, tesc, tesc_len, tese, tese_len, geom, geom_len);
  }

  ShaderProgram::~ShaderProgram() {
  }

  void ShaderProgram::m_init(const char *vert, unsigned int vert_len, const char *frag, unsigned int frag_len)
  {
    m_shaderProgram = glCreateProgram();

    GLuint vertexShader = -1;
    if (vert_len != 0) {
      vertexShader = m_compileShader(vert, vert_len, GL_VERTEX_SHADER);
      glAttachShader(m_shaderProgram, vertexShader);
    }
    GLuint fragmentShader = -1;
    if (frag_len != 0) {
      fragmentShader = m_compileShader(frag, frag_len, GL_FRAGMENT_SHADER);
      glAttachShader(m_shaderProgram, fragmentShader);
    }

    m_linkShaderProgram(m_shaderProgram);

    initLocations();
  }
  
  void ShaderProgram::m_init(const char* vert, unsigned int vert_len, const char* frag, unsigned int frag_len,
                             const char* tesc, unsigned int tesc_len, const char* tese, unsigned int tese_len,
                             const char *geom, unsigned int geom_len)
  {
    m_shaderProgram = glCreateProgram();
  
    GLuint vertexShader = -1;
    if (vert_len != 0) {
      vertexShader = m_compileShader(vert, vert_len, GL_VERTEX_SHADER);
      glAttachShader(m_shaderProgram, vertexShader);
    }
    GLuint fragmentShader = -1;
    if (frag_len != 0) {
      fragmentShader = m_compileShader(frag, frag_len, GL_FRAGMENT_SHADER);
      glAttachShader(m_shaderProgram, fragmentShader);
    }
    GLuint tessControlShader = -1;
    if (tesc_len != 0) {
      tessControlShader = m_compileShader(tesc, tesc_len, GL_TESS_CONTROL_SHADER);
      glAttachShader(m_shaderProgram, tessControlShader);
    }
    GLuint tessEvalShader = -1;
    if (tese_len != 0) {
      tessEvalShader = m_compileShader(tese, tese_len, GL_TESS_EVALUATION_SHADER);
      glAttachShader(m_shaderProgram, tessEvalShader);
    }
    GLuint geomShader = -1;
    if (geom_len != 0) {
      geomShader = m_compileShader(geom, geom_len, GL_GEOMETRY_SHADER);
      glAttachShader(m_shaderProgram, geomShader);
    }
  
    m_linkShaderProgram(m_shaderProgram);
  
    initLocations();
  }

  void ShaderProgram::m_readCode(
      const std::string &path, char **code, unsigned int *code_len)
  {
    FILE *f;
    unsigned int length;

    f = fopen(path.c_str(), "r");
    if (f == NULL) {
      fprintf(stderr, "Could not open shader file: %s\n", path.c_str());
      exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    *code_len = ftell(f);
    rewind(f);
    *code = new char[*code_len];
    length = fread(*code, 1, *code_len, f);
    if (length != *code_len) {
      fprintf(stderr, "Could not read shader file: %s\n", path.c_str());
      exit(EXIT_FAILURE);
    }
  }

  void ShaderProgram::use() const {
    glUseProgram(m_shaderProgram);
  }

  GLuint ShaderProgram::m_compileShader(
      const char *code, int code_len, GLenum type)
  {
    std::stringstream ss;
#ifndef __EMSCRIPTEN__
    ss << "\n#version " GLSL_VERSION "\n";
    code_len += strlen("\n#version " GLSL_VERSION "\n");
#endif
    ss << code;

    GLint status;

    GLuint shader = glCreateShader(type);

    std::string temp = ss.str();
    const char* finalCode = temp.c_str();

    glShaderSource(shader, 1, &finalCode, &code_len);

    glCompileShader(shader);
    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &status);
    if (status != GL_TRUE) {
      char *log;
      int logLength;
      glGetShaderiv(
          shader,
          GL_INFO_LOG_LENGTH,
          &logLength);
      log = (char*)malloc(logLength);
      glGetShaderInfoLog(
          shader,
          logLength,
          NULL,
          log);
      // FIXME: The shader path should be printed with this error
      fprintf(stderr, "Error compiling shader %s: %s\n",
        finalCode, log);
      free(log);
      // FIXME: Maybe call something other than exit() here
      exit(EXIT_FAILURE);
    }

    return shader;
  }

  void ShaderProgram::m_linkShaderProgram(GLuint shaderProgram) {
    GLint status;

    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
      char *log;
      int logLength;
      glGetProgramiv(
          shaderProgram,
          GL_INFO_LOG_LENGTH,
          &logLength);
      log = (char *)malloc(logLength);
      glGetProgramInfoLog(
          shaderProgram,
          logLength,
          NULL,
          log);
      fprintf(stderr, "Error linking shader program: %s\n", log);
      free(log);
      // FIXME: Maybe call something other than exit() here
      exit(EXIT_FAILURE);
    }
  }

  void ShaderProgram::initLocations() {
    // Register some common uniform and attribute locations
    m_modelLocation = glGetUniformLocation(m_shaderProgram, "model");
    m_modelViewLocation = glGetUniformLocation(m_shaderProgram, "modelView");
    m_projectionLocation = glGetUniformLocation(m_shaderProgram, "projection");
    m_modelViewProjectionLocation = glGetUniformLocation(m_shaderProgram, "modelViewProjection");
    m_normalTransformLocation = glGetUniformLocation(m_shaderProgram, "normalTransform");
    m_lightPositionLocation = glGetUniformLocation(m_shaderProgram, "lightPosition");
    m_lightIntensityLocation = glGetUniformLocation(m_shaderProgram, "lightIntensity");
    m_timeLocation = glGetUniformLocation(m_shaderProgram, "time");
    m_colorLocation = glGetUniformLocation(m_shaderProgram, "color");

    m_vertPositionLocation = glGetAttribLocation(m_shaderProgram, "vertPosition");
    m_vertNormalLocation = glGetAttribLocation(m_shaderProgram, "vertNormal");
    m_vertColorLocation = glGetAttribLocation(m_shaderProgram, "vertColor");
    m_vertTexCoordLocation = glGetAttribLocation(m_shaderProgram, "vertTexCoord");
    m_vertVelocityLocation = glGetAttribLocation(m_shaderProgram, "vertVelocity");
    m_vertStartTimeLocation = glGetAttribLocation(m_shaderProgram, "vertStartTime");

    m_texture0 = glGetUniformLocation(m_shaderProgram, "texture0");
    m_terrain = glGetUniformLocation(m_shaderProgram, "terrain");
    m_grass0 = glGetUniformLocation(m_shaderProgram, "grass0");
    m_cliff0 = glGetUniformLocation(m_shaderProgram, "cliff0");
    m_cliff1 = glGetUniformLocation(m_shaderProgram, "cliff1");

    m_screenSize = glGetUniformLocation(m_shaderProgram, "screen_size");
    m_mvp = glGetUniformLocation(m_shaderProgram, "mvp");
    m_lodFidelity = glGetUniformLocation(m_shaderProgram, "lod_fidelity");
    m_debugLines = glGetUniformLocation(m_shaderProgram, "debugLines");
    m_maxPatchSize = glGetUniformLocation(m_shaderProgram, "maxPatchSize");
    m_maxFieldViewDot = glGetUniformLocation(m_shaderProgram, "maxFieldViewDot");
  }
}