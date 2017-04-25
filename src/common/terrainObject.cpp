/*
 * Copyright (c) 2017 Galen Cochrane
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

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <omp.h>
#include <iostream>

#include "ezecs.hpp"
#include "glError.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "terrainObject.h"

using namespace ezecs;

namespace at3 {

  TerrainObject::TerrainObject(ezecs::State &state, const glm::mat4 &transform,
                               float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
      : SceneObject(state)
  {
    assert(xMax > xMin);
    assert(yMax > yMin);
    assert(zMax > zMin);
    float xSize = xMax - xMin;
    float ySize = yMax - yMin;
    float zSize = zMax - zMin;
    float xCenter = (xMax + xMin) * 0.5f;
    float yCenter = (yMax + yMin) * 0.5f;
    float zCenter = (zMax + zMin) * 0.5f;
    numPatchesX = std::max((size_t)1, (size_t)(xSize / maxPatchSize));
    numPatchesY = std::max((size_t)1, (size_t)(ySize / maxPatchSize));

    // TODO: dynamic res and fidelity picking?

    m_genMesh();
    glm::vec2 newZInfo = m_genMaps(xSize, ySize, zSize);
    std::cout << newZInfo.x << " -> " << newZInfo.y << std::endl;
    std::cout << (newZInfo.x + newZInfo.y) * 0.5f << ", " << newZInfo.y - newZInfo.x << std::endl;
    float newZSize = zSize / (newZInfo.y - newZInfo.x);
    float newZCenter = zCenter - ((newZInfo.x + newZInfo.y) * 0.5f);
    std::cout << newZCenter << ", " << newZSize << std::endl;

    glm::mat4 translated = glm::translate(transform, {xCenter, yCenter, zCenter});

    ezecs::CompOpReturn status;
    status = this->state->add_Placement(id, translated);
    assert(status == ezecs::SUCCESS);

    status = this->state->add_Terrain(id, &heights, resX, resY, xSize, ySize, newZSize, newZInfo.x, newZInfo.y);
    assert(status == ezecs::SUCCESS);

    status = this->state->add_Physics(id, 0.f, NULL, Physics::TERRAIN);
    assert(status == ezecs::SUCCESS);

    // at this point system callbacks have fired, and it's safe to set a scale matrix FIXME: This hack.
    glm::mat4 scaledTransform = glm::scale(translated, {xSize, ySize, newZSize});
    glm::mat4 centeredTransform = glm::translate(scaledTransform, {0.f, 0.f, (newZInfo.x + newZInfo.y) * -0.5f});

    std::cout << centeredTransform[3][2] << std::endl;
    Placement *placement;
    this->state->get_Placement(id, &placement);
    placement->mat = centeredTransform;

  }

  TerrainObject::~TerrainObject() { }

  void TerrainObject::m_genMesh() {
    size_t strideY = numPatchesX + 1;
    std::vector<float> verts;
    for (size_t y = 0; y < numPatchesY + 1; ++y) {
      for (size_t x = 0; x < numPatchesX + 1; ++x) { // for each vertex
        verts.push_back( (float)x / (float)numPatchesX - 0.5f);
        verts.push_back( (float)y / (float)numPatchesY - 0.5f);
      }
    }
    std::vector<uint16_t> indices;
    for (size_t y = 0; y < numPatchesY; ++y) {
      for (size_t x = 0; x < numPatchesX; ++x) { // for each quad
        indices.push_back((uint16_t)(strideY *  y      + x    ));
        indices.push_back((uint16_t)(strideY * (y + 1) + x    ));
        indices.push_back((uint16_t)(strideY * (y + 1) + x + 1));
        indices.push_back((uint16_t)(strideY *  y      + x + 1));
      }
    }

    // Copy the vertices buffer to the GL
    glGenBuffers(1, &m_vertexBuffer);                              FORCE_ASSERT_GL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);                 FORCE_ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,  // target
        sizeof(float) * verts.size(),  // size
        verts.data(),  // data
        GL_STATIC_DRAW  // usage
    );                                                             FORCE_ASSERT_GL_ERROR();
    // Copy the index data to the GL
    glGenBuffers(1, &m_indexBuffer);                               FORCE_ASSERT_GL_ERROR();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);          FORCE_ASSERT_GL_ERROR();
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,  // target
        sizeof(uint16_t) * indices.size(),  // size
        indices.data(),  // data
        GL_STATIC_DRAW  // usage
    );                                                             FORCE_ASSERT_GL_ERROR();
    m_numIndices = indices.size();
  }

  float TerrainObject::m_genTrigTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain,
                                        float xScale, float yScale, float zScale) {
    assert(false); // This function is out of date, do not use.
    float hillFreqX = 1.f / ((float)resX / 10.f);
    float hillFreqY = 1.f / ((float)resY / 12.f);
    float hNormalizer = 1 / (1 + 1 + 0.2f + 0.18f);
    float newZScale = hNormalizer * (0.5f * zScale);
    for (GLsizei y = 0; y < resY; ++y) {
      for (GLsizei x = 0; x < resX; ++x) {
        float height = hNormalizer * ( // 0.0625f
            sin((float)x * hillFreqX) + 0.2f * cos((float)x * hillFreqX * 3.7f) +
            sin((float)y * hillFreqY) + 0.18f * sin((float)y * hillFreqY * 2.8f)
        );
        glm::vec3 normal( // FIXME: fix this normal to scale
            (zScale / xScale) * (cos((float)x * hillFreqX) - 3.7f * 0.2f * sin((float)x * hillFreqX * 3.7f)),
            (zScale / yScale) * (cos((float)y * hillFreqY) + 2.8f * 0.18f * cos((float)y * hillFreqY * 2.8f)),
            1.f
        );
        normal = glm::normalize(normal);
        // fill terrain texture data
        terrain.push_back(normal.x);
        terrain.push_back(normal.y);
        terrain.push_back(normal.z);
        terrain.push_back(height);
        // fill diffuse texture data
        diffuse.push_back((uint8_t)((1.f - normal.z) * 96 + 32));
        diffuse.push_back((uint8_t)(normal.z * 32 + 96));
        diffuse.push_back((uint8_t)((1.f - normal.z) * 64 + 32));
        diffuse.push_back(255);
        // keep the terrain heights around for physics
        heights.push_back(height);
      }
    }
    return newZScale;
  }

  float TerrainObject::m_getNoise(float x, float y) {

    noiseGen.SetNoiseType(FastNoise::SimplexFractal);
    noiseGen.SetFractalType(FastNoise::FBM);
    noiseGen.SetInterp(FastNoise::Quintic);
    float value = 2.f * (
        noiseGen.GetNoise(x, y) * 0.5f +
        noiseGen.GetNoise(x * 0.5f /*+ 15000*/, y * 0.5f /*+ 15000*/) +
        noiseGen.GetNoise(x * 0.25f /*- 1500*/, y * 0.25f /*- 1500*/));
    noiseGen.SetNoiseType(FastNoise::SimplexFractal);
    noiseGen.SetFractalType(FastNoise::Billow);
    noiseGen.SetInterp(FastNoise::Quintic);
    value *= (1.f - noiseGen.GetNoise(x * 0.2f, y * 0.2f));

    return value;
  }

  glm::vec2 TerrainObject::m_genTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain, float xScale,
                                        float yScale, float zScale) {
    // TODO: make use of zScale
    bool minMaxInit = false;
    float min = 0, max = 0;
    float ds = 1.0f;
    GLsizei y, x;
//    #pragma omp parallel for private(y, x)
    for (y = 0; y < resY; ++y) {
      for (x = 0; x < resX; ++x) {
        float nx = x * xScale * 0.05f / resX;
        float ny = y * yScale * 0.05f / resY;
        float height = m_getNoise(nx,ny);
        // FIXME: normal calculation is not physically based and is wrong (should be ds * 2 for x and y components)?
        glm::vec3 normal = glm::cross(
            glm::vec3(ds /** 0.12f*/, 0.f, m_getNoise(nx + ds, ny) - m_getNoise(nx - ds, ny)),
            glm::vec3(0.f, ds /** 0.12f*/, m_getNoise(nx, ny + ds) - m_getNoise(nx, ny - ds))
        );
        normal = glm::normalize(normal);
        // fill terrain texture data
        terrain.at(((y * resX) + x) * 4 + 0) = normal.x;
        terrain.at(((y * resX) + x) * 4 + 1) = normal.y;
        terrain.at(((y * resX) + x) * 4 + 2) = normal.z;
        terrain.at(((y * resX) + x) * 4 + 3) = height;
        // fill diffuse texture data
        diffuse.at(((y * resX) + x) * 4 + 0) = (uint8_t)(255.f * abs(normal.x));
        diffuse.at(((y * resX) + x) * 4 + 1) = (uint8_t)(255.f * abs(normal.y));
        diffuse.at(((y * resX) + x) * 4 + 2) = (uint8_t)(96.f  * abs(normal.z));
        diffuse.at(((y * resX) + x) * 4 + 3) = 255;
        // keep the terrain heights around for physics
        heights.at((y * resX) + x) = height;
        if (!minMaxInit) {
          min = height;
          max = height;
          minMaxInit = true;
        } else {
          min = std::min(min, height);
          max = std::max(max, height);
        }
      }
    }
    return glm::vec2(min, max);
  }

  glm::vec2 TerrainObject::m_genMaps(float xScale, float yScale, float zScale) {

    std::vector<float> terrain;
    std::vector<uint8_t> diffuse;

    heights.resize(resY * resX);
    terrain.resize(resY * resX * 4);
    diffuse.resize(resY * resX * 4);

    glm::vec2 newZInfo = m_genTerrain(diffuse, terrain, xScale, yScale, zScale);

    // Create the terrain texture object in the GL
    glGenTextures(1, &m_terrain);                                            FORCE_ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_terrain);                                 FORCE_ASSERT_GL_ERROR();
    // Copy the image to the GL
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        0,  // level
        GL_RGBA32F,  // internal format
        (GLsizei)resX,  // width
        (GLsizei)resY,  // height
        0,  // border
        GL_RGBA,  // format
        GL_FLOAT,  // type
        terrain.data()  // data
    );                                                                       FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();

    // Create the diffuse texture object in the GL
    glGenTextures(1, &m_diffuse);                                            FORCE_ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_diffuse);                                 FORCE_ASSERT_GL_ERROR();
    // Copy the image to the GL
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        0,  // level
        GL_RGBA,  // internal format
        (GLsizei)resX,  // width
        (GLsizei)resY,  // height
        0,  // border
        GL_RGBA,  // format
        GL_UNSIGNED_BYTE,  // type
        diffuse.data()  // data
        );                                                                   FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();

    return newZInfo;
  }

  void TerrainObject::m_drawSurface(
      const glm::mat4 &modelView,
      const glm::mat4 &projection)
  {
    // Use a simple shader
    auto shader = Shaders::terrainShader();
    shader->use();

    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(modelView)  // value
    );                                                                 ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(projection)  // value
    );                                                                 ASSERT_GL_ERROR();
    assert(shader->mvp() != -1);
    glUniformMatrix4fv(
        shader->mvp(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(projection * modelView)  // value
    );                                                                 ASSERT_GL_ERROR();
    assert(shader->lodFidelity() != -1);
    glUniform1f(shader->lodFidelity(), lodFidelity);                   ASSERT_GL_ERROR();

    assert(shader->maxPatchSize() != -1);
    glUniform1f(shader->maxPatchSize(), maxPatchSize);                 ASSERT_GL_ERROR();

    // NOTE: screen size uniform is set on window size change events (game.cpp)

    // Prepare the diffuse texture sampler
    assert(shader->texture0() != -1);
    glUniform1i(
        shader->texture0(),  // location
        0  // value
        );                                                             ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE0);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_diffuse);                           ASSERT_GL_ERROR();

    // Prepare the terrain texture sampler
    assert(shader->terrain() != -1);
    glUniform1i(
        shader->terrain(),  // location
        1  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE1);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_terrain);                           ASSERT_GL_ERROR();

    // Prepare the diffuse texture sampler
    assert(shader->grass0() != -1);
    glUniform1i(
        shader->grass0(),  // location
        2  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE2);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, grass.get());                         ASSERT_GL_ERROR();

    // Prepare the diffuse texture sampler
    assert(shader->cliff0() != -1);
    glUniform1i(
        shader->cliff0(),  // location
        3  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE3);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, cliff0.get());                        ASSERT_GL_ERROR();

    // Prepare the diffuse texture sampler
    assert(shader->cliff1() != -1);
    glUniform1i(
        shader->cliff1(),  // location
        4  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE4);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, cliff1.get());                        ASSERT_GL_ERROR();

    // Prepare the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);                     ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());         ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(),  // index
        2,                 // size
        GL_FLOAT,          // type
        0,                 // normalize
        sizeof(float) * 2, // stride
        0                  // offset
    );                                                                 ASSERT_GL_ERROR();

    // Draw the surface
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawElements(
        GL_PATCHES,  // mode
        m_numIndices,  // count
        GL_UNSIGNED_SHORT,  // type
        0  // indices
    );                                                                 ASSERT_GL_ERROR();
  }

  void TerrainObject::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
      bool debug)
  {
    glm::mat4 modelView = worldView * modelWorld;
    if (state->getComponents(id) & TRANSFORMFUNCTION) {
      TransformFunction* transformFunction;
      state->get_TransformFunction(id, &transformFunction);
      modelView *= transformFunction->transformed;
    }
    m_drawSurface(modelView, projection);
  }

  LoadedTexture TerrainObject::grass;
  LoadedTexture TerrainObject::cliff0;
  LoadedTexture TerrainObject::cliff1;

  bool TerrainObject::initTextures() {
    grass = LoadedTexture("assets/textures/grass1024_00.jpg");
    cliff0 = LoadedTexture("assets/textures/cliff1024_00.jpg");
    cliff1 = LoadedTexture("assets/textures/cliff1024_01.jpg");
    return true;
  }
}
