/*
 * Copyright (c) 2016 Jonathan Glines
 * Jonathan Glines <jonathan@glines.net>
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

#include "sceneObject.h"

namespace at3 {
  class TerrainObject : public SceneObject {
    private:
      GLuint m_vertexBuffer, m_indexBuffer,  m_diffuse, m_terrain;
      size_t m_numIndices;
      float lodFidelity = 0.02f, maxPatchSize = 200;
      size_t numPatchesX, numPatchesY, resX = 512, resY = 512;
      std::vector<float> heights;

      void m_genMesh();
      float m_genTextures(float yScale, float xScale, float zScale);

      void m_drawSurface(
          const glm::mat4 &modelView,
          const glm::mat4 &projection);
    public:
      TerrainObject(ezecs::State &state, const glm::mat4 &transform,
                    float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
      virtual ~TerrainObject();

      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                        bool debug);
  };
}

#endif