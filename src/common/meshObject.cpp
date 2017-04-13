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

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"

#include "ezecs.hpp"
#include "glError.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "meshObject.h"

using namespace ezecs;

namespace at3 {
  MeshObject::MeshObject(ezecs::State &state, const std::string &meshFile, const std::string &textureFile,
                           glm::mat4 &transform)
      : SceneObject(state)
  {
    ezecs::CompOpReturn status;
    status = this->state->add_Placement(id, transform);
    assert(status == ezecs::SUCCESS);

    // Load the mesh from file using assimp
    m_loadMesh(meshFile);
    // Load the texture from file using SDL2
    m_loadTexture(textureFile);
  }

  MeshObject::~MeshObject() {
  }

  void MeshObject::m_loadMesh(const std::string &meshFile) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(
        meshFile,
        aiProcess_Triangulate
        );

    if (!scene) {
      fprintf(stderr, "Failed to load mesh from file: '%s'\n",
          meshFile.c_str());
    }
    if (scene->mNumMeshes != 1) {
      fprintf(stderr, "Mesh file '%s' must contain a single mesh",
          meshFile.c_str());
    }
    auto aim = scene->mMeshes[0];
    if (aim->mNormals == NULL) {
      fprintf(stderr, "Error: No normals in mesh '%s'\n",
          meshFile.c_str());
    }
    if (!aim->HasTextureCoords(0)) {
      fprintf(stderr, "Error: No texture coordinates in mesh '%s'\n",
          meshFile.c_str());
    }

    // Copy the mesh vertices into a buffer with the appropriate format
    MeshVertex *vertices = new MeshVertex[aim->mNumVertices];
    for (int i = 0; i < aim->mNumVertices; ++i) {
      vertices[i].pos[0] = aim->mVertices[i].x;
      vertices[i].pos[1] = aim->mVertices[i].y;
      vertices[i].pos[2] = aim->mVertices[i].z;
      vertices[i].norm[0] = aim->mNormals[i].x;
      vertices[i].norm[1] = aim->mNormals[i].y;
      vertices[i].norm[2] = aim->mNormals[i].z;
      vertices[i].tex[0] = aim->mTextureCoords[0][i].x;
      vertices[i].tex[1] = aim->mTextureCoords[0][i].y;
      /*fprintf(stderr,
          "vertices[%d]:\n"
          "  pos: (%g, %g, %g)\n"
          "  norm: (%g, %g, %g)\n"
          "  tex: (%g, %g)\n",
          i,
          vertices[i].pos[0],
          vertices[i].pos[1],
          vertices[i].pos[2],
          vertices[i].norm[0],
          vertices[i].norm[1],
          vertices[i].norm[2],
          vertices[i].tex[0],
          vertices[i].tex[1]);*/
    }
    // Copy the vertices buffer to the GL
    glGenBuffers(1, &m_vertexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,  // target
        sizeof(MeshVertex) * aim->mNumVertices,  // size
        vertices,  // data
        GL_STATIC_DRAW  // usage
        );
    FORCE_ASSERT_GL_ERROR();
    delete[] vertices;
    // Copy the face data into an index buffer
    uint32_t *indices = new uint32_t[3 * aim->mNumFaces];
    for (int i = 0; i < aim->mNumFaces; ++i) {
      assert(aim->mFaces[i].mNumIndices == 3);
      indices[i * 3] = aim->mFaces[i].mIndices[0];
      indices[i * 3 + 1] = aim->mFaces[i].mIndices[1];
      indices[i * 3 + 2] = aim->mFaces[i].mIndices[2];
    }
    // Copy the index data to the GL
    glGenBuffers(1, &m_indexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,  // target
        sizeof(uint32_t) * 3 * aim->mNumFaces,  // size
        indices,  // data
        GL_STATIC_DRAW  // usage
        );
    FORCE_ASSERT_GL_ERROR();
    delete[] indices;
    m_numIndices = aim->mNumFaces * 3;
  }

  void MeshObject::m_loadTexture(const std::string &textureFile) {
    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(textureFile.c_str(), &x, &y, &n, 0);
    if (!data) {
      fprintf(stderr, "Failed to load texture file '%s'.\n", textureFile.c_str());
    }
    // Create the texture object in the GL
    glGenTextures(1, &m_texture);
    FORCE_ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_texture);
    FORCE_ASSERT_GL_ERROR();
    // Copy the image to the GL
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        0,  // level
        GL_RGBA,  // internal format
        x,  // width
        y,  // height
        0,  // border
        GL_RGBA,  // format
        GL_UNSIGNED_BYTE,  // type
        data  // data
        );
    FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    FORCE_ASSERT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    FORCE_ASSERT_GL_ERROR();
    stbi_image_free(data);
  }

  void MeshObject::m_drawSurface(
      const glm::mat4 &modelView,
      const glm::mat4 &projection)
  {
    // Use a simple shader
//    auto shader = Shaders::textureShader();
    auto shader = Shaders::terrainShader();
    shader->use();

    // Prepare the uniform values
    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(modelView)  // value
        );
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(projection)  // value
        );
    ASSERT_GL_ERROR();

    /*
    // Prepare the texture sampler
    assert(shader->texture0() != -1);
    glUniform1i(
        shader->texture0(),  // location
        0  // value
        );
    ASSERT_GL_ERROR();
    glActiveTexture(GL_TEXTURE0);
    ASSERT_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, m_texture);
    ASSERT_GL_ERROR();
    */

    // Prepare the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(MeshVertex),  // stride
        &(((MeshVertex *)0)->pos[0])  // pointer
        );
    ASSERT_GL_ERROR();
    /*
    assert(shader->vertNormalLocation() != -1);
    glEnableVertexAttribArray(shader->vertNormalLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertNormalLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(MeshVertex),  // stride
        &(((MeshVertex *)0)->norm[0])  // pointer
        );
    ASSERT_GL_ERROR();
    */
    assert(shader->vertTexCoordLocation() != -1);
    glEnableVertexAttribArray(shader->vertTexCoordLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertTexCoordLocation(),  // index
        2,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(MeshVertex),  // stride
        &(((MeshVertex *)0)->tex[0])  // pointer
        );
    ASSERT_GL_ERROR();

    // Draw the surface
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    ASSERT_GL_ERROR();
    glDrawElements(
        GL_TRIANGLES,  // mode
        m_numIndices,  // count
        GL_UNSIGNED_INT,  // type
        0  // indices
        );
    ASSERT_GL_ERROR();
  }

  void MeshObject::draw(const glm::mat4 &modelWorld,
      const glm::mat4 &worldView, const glm::mat4 &projection,
      float alpha, bool debug)
  {
//    ezecs::Scale* scale;
//    state->get_Scale(id, &scale);
    glm::mat4 modelView = worldView * modelWorld;
    if (state->getComponents(id) & TRANSFORMFUNCTION) {
      TransformFunction* transformFunction;
      state->get_TransformFunction(id, &transformFunction);
      modelView *= transformFunction->transformed;
    }
    m_drawSurface(modelView, projection);
  }
}
