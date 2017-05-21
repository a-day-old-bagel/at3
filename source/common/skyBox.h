#pragma once

#include <epoxy/gl.h>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "sceneObject.h"
#include "loadCubeMap.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "glError.h"

namespace at3 {
  template <typename EcsInterface>
  class SkyBox : public SceneObject<EcsInterface> {
      GLuint vertices, texture;
      void m_drawSurface( const glm::mat4 &modelView, const glm::mat4 &projection);
    public:
      SkyBox();
      virtual ~SkyBox();
      LoadResult useCubeMap(std::string fileName, std::string fileType);
      virtual void draw(const glm::mat4 &modelWorld,
                        const glm::mat4 &worldView, const glm::mat4 &projection, bool debug);
  };

  template <typename EcsInterface>
  SkyBox<EcsInterface>::SkyBox() {
    // generate a triangle that covers the screen
    float corners[9] = {-1.0f, -1.0f, 1.f,
                        3.0f, -1.0f, 1.f,
                        -1.0f,  3.0f, 1.f,
    };
    glGenBuffers(1, &vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, corners, GL_STATIC_DRAW);
    // generate texture
    glGenTextures(1, &texture);
  }

  template <typename EcsInterface>
  SkyBox<EcsInterface>::~SkyBox() {

  }

  template <typename EcsInterface>
  LoadResult SkyBox<EcsInterface>::useCubeMap(std::string fileName, std::string fileType) {
    LoadResult result = readAndBufferCubeMap(
        std::string("assets/cubeMaps/" + fileName + "/negz." + fileType).c_str(),
        std::string("assets/cubeMaps/" + fileName + "/posz." + fileType).c_str(),
        std::string("assets/cubeMaps/" + fileName + "/negy." + fileType).c_str(),
        std::string("assets/cubeMaps/" + fileName + "/posy." + fileType).c_str(),
        std::string("assets/cubeMaps/" + fileName + "/negx." + fileType).c_str(),
        std::string("assets/cubeMaps/" + fileName + "/posx." + fileType).c_str(),
        &texture);
    if (result != LOAD_SUCCESS) {
      return result;
    }
    // format texture
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return LOAD_SUCCESS;
  }

  template <typename EcsInterface>
  void SkyBox<EcsInterface>::draw(const glm::mat4 &modelWorld,
                                         const glm::mat4 &worldView, const glm::mat4 &projection, bool debug) {
    glm::mat4 axesCorrection = glm::rotate((float)(M_PI * 0.5f), glm::vec3(1.f, 0.f, 0.f));
    glm::mat4 reverseView = axesCorrection * glm::transpose(worldView);
    glm::mat4 inverseProj = glm::inverse(projection);
    m_drawSurface(reverseView, inverseProj);
  }

  template <typename EcsInterface>
  void SkyBox<EcsInterface>::m_drawSurface(const glm::mat4 &modelView, const glm::mat4 &projection) {
    auto shader = Shaders::skyQuadShader();
    shader->use();

    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(shader->modelViewLocation(), 1, 0, glm::value_ptr(modelView));
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(shader->projectionLocation(), 1, 0, glm::value_ptr(projection));
    ASSERT_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(shader->vertPositionLocation(), 3, GL_FLOAT, GL_FALSE, 0, 0);
    ASSERT_GL_ERROR();

    assert(shader->texture0() != -1);
    glUniform1i(shader->texture0(), 0);
    ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE0);
    ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    ASSERT_GL_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 3);
    ASSERT_GL_ERROR();
  }
}
