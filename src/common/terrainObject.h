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

#ifndef LD2016_COMMON_TERRAIN_OBJECT_H_
#define LD2016_COMMON_TERRAIN_OBJECT_H_

#include <epoxy/gl.h>
#include <string>
#include <FastNoise.h>

#include "sceneObject.h"

namespace at3 {
  class TerrainObject : public SceneObject {
    private:
      GLuint m_vertexBuffer, m_indexBuffer,  m_diffuse, m_terrain;
      size_t m_numIndices;
      float lodFidelity = 0.02f, maxPatchSize = 150;
      size_t numPatchesX, numPatchesY, resX = 2048, resY = 2048;
      std::vector<float> heights;

      void m_genMesh();
      glm::vec2 m_genMaps(float xScale, float yScale, float zScale);

      void m_drawSurface(
          const glm::mat4 &modelView,
          const glm::mat4 &projection);

      float m_genTrigTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain,
                             float xScale, float yScale, float zScale);

      FastNoise noiseGen;
      float m_getNoise(float x, float y);
      glm::vec2 m_genTerrain(std::vector<uint8_t> &diffuse, std::vector<float> &terrain,
                             float xScale, float yScale, float zScale);

    public:
      TerrainObject(ezecs::State &state, const glm::mat4 &transform,
                    float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
      virtual ~TerrainObject();

      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                        bool debug);
  };
}

#endif
