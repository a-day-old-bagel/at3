/*
 * Copyright (c) 2016 Galen Cochrane
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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "skyBox.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "glError.h"

namespace ld2016 {

  SkyBox::SkyBox(ezecs::State &state) : SceneObject(state) {
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
  SkyBox::~SkyBox() {

  }
  LoadResult SkyBox::useCubeMap(std::string fileName, std::string fileType) {
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
  void SkyBox::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection, float alpha,
                    bool debug) {
    glm::mat4 axesCorrection = glm::rotate((float)(M_PI * 0.5f), glm::vec3(1.f, 0.f, 0.f));
    glm::mat4 reverseView = axesCorrection * glm::transpose(worldView);
    glm::mat4 inverseProj = glm::inverse(projection);
    m_drawSurface(reverseView, inverseProj);
  }
  void SkyBox::m_drawSurface(const glm::mat4 &modelView, const glm::mat4 &projection) {
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