#pragma once

#include <epoxy/gl.h>
#include <string>
#include <FastNoise.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <omp.h>
#include <iostream>
#include <vector>

#include "sceneObject.h"
#include "loadedTexture.h"
#include "openglValidation.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "textureView.h"
#include "imageProcessing.h"

namespace at3 {
  template <typename EcsInterface>
  class TerrainObject : public SceneObject<EcsInterface> {
    private:
      GLuint m_vertexBuffer, m_indexBuffer, m_diffuse, m_terrain, m_test;
      size_t m_numIndices;
      float lodFidelity = 0.02f, maxPatchSize = 150;
      float noiseCenterX, noiseCenterY;
      size_t numPatchesX, numPatchesY, resX = 2048, resY = 2048;
      Image<float> heights;
      Image<float> edges;
      TextureView textureView;

      // TODO: move these into some kind of texture repo.
      static std::shared_ptr<LoadedTexture> grass;
      static std::shared_ptr<LoadedTexture> cliff0;
      static std::shared_ptr<LoadedTexture> cliff1;

      void m_genMesh();
      glm::vec2 m_genMaps(float xScale, float yScale, float zScale);

      void m_drawSurface(
          const glm::mat4 &modelView,
          const glm::mat4 &projection);

      FastNoise noiseGen;
      float m_getHeight(float x, float y);
      glm::vec2 m_genTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain,
                             float xScale, float yScale, float zScale);

    public:
      TerrainObject(const glm::mat4 &transform,
                    float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
      virtual ~TerrainObject();

      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                        bool debug);

      static bool initTextures();
  };

  template <typename EcsInterface>
  TerrainObject<EcsInterface>::TerrainObject(const glm::mat4 &transform,
                                             float xMin, float xMax, float yMin, float yMax, float zMin, float zMax) {
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
    float newZSize = zSize / (newZInfo.y - newZInfo.x);

    glm::mat4 translated = glm::translate(transform, {xCenter, yCenter, zCenter});

    SCENE_ECS->addTerrain(SCENE_ID, translated, heights.getValues(),
                          resX, resY, xSize, ySize, newZSize, newZInfo.x, newZInfo.y);

    // at this point system callbacks have fired, and it's safe to set a scale matrix FIXME: This hack.
    glm::mat4 scaledTransform = glm::scale(translated, {xSize, ySize, newZSize});
    glm::mat4 centeredTransform = glm::translate(scaledTransform, {0.f, 0.f, (newZInfo.x + newZInfo.y) * -0.5f});

    SCENE_ECS->setTransform(SCENE_ID, centeredTransform);

    textureView.viewTextureUnit(5);

  }

  template <typename EcsInterface>
  TerrainObject<EcsInterface>::~TerrainObject() { }

  template <typename EcsInterface>
  void TerrainObject<EcsInterface>::m_genMesh() {
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

  template <typename EcsInterface>
  float TerrainObject<EcsInterface>::m_getHeight(float x, float y) {

    // noiseGen.GetNoise generally returns between -1 and 1.

    // odd outcroppings of rock form a maze
    float minWallHeight = 3.f;
    noiseGen.SetNoiseType(FastNoise::Cellular);
    noiseGen.SetFractalType(FastNoise::Billow);
    noiseGen.SetInterp(FastNoise::Quintic);
    float value = noiseGen.GetNoise(x * 20.f, y * 20.f);
    value = (value > 0.25f) ? std::max(value * minWallHeight * 2, minWallHeight) : 0.f;

    // Add shape to tops of outcroppings
    noiseGen.SetNoiseType(FastNoise::SimplexFractal);
    value = value + value * (noiseGen.GetNoise(x * 5.f, y * 5.f) + 1.f);

    // create the crater area
    float distToCenterX = noiseCenterX - x;
    float distToCenterY = noiseCenterY - y;
    float radius = sqrt(distToCenterX * distToCenterX + distToCenterY * distToCenterY);
    float craterRadius = 50.f;
    float fringeCraterRadius = craterRadius * 2.f;
    float cleanCraterRadius = craterRadius * 0.95f;
    float dirtyCraterRadius = craterRadius * 1.15f;
    if (radius < fringeCraterRadius) {
      // This will be filled in below and become the final value.
      float craterValue = 0.f;
      // a shallow parabolic crater to be applied to walkable terrain
      float craterDepression = -(craterRadius * craterRadius - radius * radius) * 0.005f;
      if (value) {
        // a slightly deeper parabolic crater offset upwards to be applied to outcroppings at the fringe
        float outCrop = std::min(value, -(cleanCraterRadius * cleanCraterRadius - radius * radius) * 0.0052f);
        if (radius > dirtyCraterRadius) {
          craterValue = outCrop;
        } else if (radius < cleanCraterRadius) {
          craterValue = craterDepression;
        } else {
          float interp =(-cos((float)M_PI * ( (radius - cleanCraterRadius)
                                              / (dirtyCraterRadius - cleanCraterRadius))) + 1.f) * 0.5f;
          craterValue = (1.f - interp) * craterDepression + interp * outCrop;
        }
      } else {
        craterValue = std::min(craterDepression, value);
        if (radius > craterRadius) {
          // add a debris ring by elevating walkable terrain near the fringe
          float debrisRadiusFactor = 2.f; // widens the berm.
          float debrisRaduis = (float)M_PI * debrisRadiusFactor; // radius of berm (peak to edge)
          float distFromDebris = fabs(craterRadius + debrisRaduis - radius); // from peak
          if (distFromDebris < debrisRaduis) {
            float interp = (radius - craterRadius) / (debrisRaduis * 2);
            craterValue = interp * (cos(distFromDebris / debrisRadiusFactor) + 1.f)
                        + (1.f - interp) * craterDepression;
          }
        }
      }
      value = craterValue;
    } // crater area finished

    // Add smooth rolling shape to all areas of map
    noiseGen.SetFractalType(FastNoise::FBM);
    value += 2.f * noiseGen.GetNoise(x, y);

    return value;
  }

# define AT(x,y) (((y) * resX) + (x))

  template <typename EcsInterface>
  glm::vec2 TerrainObject<EcsInterface>::m_genTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain,
                                                             float xScale, float yScale, float zScale) {

    // TODO: make use of zScale
    bool minMaxInit = false;
    float min = 0, max = 0;
    size_t ds = 1;
    size_t y, x;

    float noiseScaleFactor = 0.05f;

    // Set point where the center of the crater will be formed in terrain (for maze maps)
    noiseCenterX = 0.5f * xScale * noiseScaleFactor;
    noiseCenterY = 0.5f * yScale * noiseScaleFactor;

    // Generate heights at each (x, y)
    for (y = 0; y < resY; ++y) {
      for (x = 0; x < resX; ++x) {
        float nx = x * xScale * noiseScaleFactor / resX;
        float ny = y * yScale * noiseScaleFactor / resY;
        float height = m_getHeight(nx, ny);
        heights.at(x,y) = height;
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

    // Scale heights to match the desired min/max
    float zScaleFactor = zScale / (max - min);
    for (auto & height : *heights.getValues()) {
      height -= min;
      height *= zScaleFactor;
    }
    max = (max - min) * zScaleFactor;
    min = 0;

    Image<float> gradX = sobelGradientX(heights);
    Image<float> gradY = sobelGradientY(heights);

    // Use heights to generate normal and diffuse color at each (x, y)
    for (y = 0; y < resY; ++y) {
      for (x = 0; x < resX; ++x) {
        // FIXME: check normals for correctness
        glm::vec3 normal = glm::vec3(gradX.at(x, y), gradY.at(x, y), 2.f);

        normal = glm::normalize(normal);
        // fill terrain texture data
        terrain.at(AT(x, y) * 4 + 0) = normal.x;
        terrain.at(AT(x, y) * 4 + 1) = normal.y;
        terrain.at(AT(x, y) * 4 + 2) = normal.z;
        terrain.at(AT(x, y) * 4 + 3) = heights.at(x,y);
        // fill diffuse texture data
        diffuse.at(AT(x, y) * 4 + 0) = (uint8_t) (255.f * abs(normal.x));
        diffuse.at(AT(x, y) * 4 + 1) = (uint8_t) (255.f * abs(normal.y));
        diffuse.at(AT(x, y) * 4 + 2) = (uint8_t) (96.f * abs(normal.z));
        diffuse.at(AT(x, y) * 4 + 3) = 255;
      }
    }

    edges = sobelEdgeMono(gradX, gradY, 10);

    // Return min/max of heights FIXME: This should no longer be necessary since it's already scaled above.
    return glm::vec2(min, max);
  }

# undef AT

  template <typename EcsInterface>
  glm::vec2 TerrainObject<EcsInterface>::m_genMaps(float xScale, float yScale, float zScale) {

    std::vector<float> terrain;
    std::vector<uint8_t> diffuse;

    heights.resize(resX, resY);
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
    );                                                                       FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();


    // Create the terrain texture object in the GL
    glGenTextures(1, &m_test);                                               FORCE_ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_test);                                    FORCE_ASSERT_GL_ERROR();
    // Copy the image to the GL
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        0,  // level
        GL_R32F,  // internal format
        (GLsizei)resX,  // width
        (GLsizei)resY,  // height
        0,  // border
        GL_RED,  // format
        GL_FLOAT,  // type
        edges.data()  // data
    );                                                                       FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);        FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);     FORCE_ASSERT_GL_ERROR();

    return newZInfo;
  }

  template <typename EcsInterface>
  void TerrainObject<EcsInterface>::m_drawSurface(
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
    );                                                                 ASSERT_GL_ERROR();
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
    glBindTexture(GL_TEXTURE_2D, grass->get());                        ASSERT_GL_ERROR();

    // Prepare the diffuse texture sampler
    assert(shader->cliff0() != -1);
    glUniform1i(
        shader->cliff0(),  // location
        3  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE3);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, cliff0->get());                       ASSERT_GL_ERROR();

    // Prepare the diffuse texture sampler
    assert(shader->cliff1() != -1);
    glUniform1i(
        shader->cliff1(),  // location
        4  // value
    );                                                                 ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE4);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, cliff1->get());                       ASSERT_GL_ERROR();

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

    glActiveTexture(GL_TEXTURE5);                                      ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_test);
    textureView.draw();
  }

  template <typename EcsInterface>
  void TerrainObject<EcsInterface>::draw(const glm::mat4 &modelWorld,
                                                const glm::mat4 &worldView, const glm::mat4 &projection, bool debug)
  {
    glm::mat4 modelView = worldView * modelWorld;
    if (SCENE_ECS->hasTransformFunction(SCENE_ID)) {
      modelView *= SCENE_ECS->getTransformFunction(SCENE_ID);
    }
    m_drawSurface(modelView, projection);
  }

  template <typename EcsInterface>
  std::shared_ptr<LoadedTexture> TerrainObject<EcsInterface>::grass;
  template <typename EcsInterface>
  std::shared_ptr<LoadedTexture> TerrainObject<EcsInterface>::cliff0;
  template <typename EcsInterface>
  std::shared_ptr<LoadedTexture> TerrainObject<EcsInterface>::cliff1;

  template <typename EcsInterface>
  bool TerrainObject<EcsInterface>::initTextures() {
    grass = std::make_shared<LoadedTexture>("assets/textures/grass1024_00.jpg", LoadedTexture::SINGLE);
    cliff0 = std::make_shared<LoadedTexture>("assets/textures/cliff1024_00.jpg", LoadedTexture::SINGLE);
    cliff1 = std::make_shared<LoadedTexture>("assets/textures/cliff1024_01.jpg", LoadedTexture::SINGLE);
    return true;
  }
}
