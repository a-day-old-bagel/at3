#pragma once

#include <epoxy/gl.h>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"

#include "sceneObject.h"
#include "glUtil.h"
#include "shaderProgram.h"
#include "shaders.h"
#include "meshObject.h"

namespace at3 {
  template <typename EcsInterface>
  class MeshObject : public SceneObject<EcsInterface> {
    private:

      typedef struct {
        float pos[3];
        float norm[3];
        float tex[2];
      } MeshVertex;
      typedef struct {
        float pos[3];
        float norm[3];
      } MeshVertex_NoTex;
      typedef struct {
        float pos[3];
        float tex[2];
      } MeshVertex_NoNorm;

      int shaderStyle;

      std::string m_meshFile;
      GLuint m_vertexBuffer, m_indexBuffer, m_texture;
      int m_numIndices;

      void m_loadMesh(const std::string &meshFile);
      void m_loadTexture(const std::string &textureFile);

      void m_drawSurface(
          const glm::mat4 &model,
          const glm::mat4 &modelView,
          const glm::mat4 &projection);
    public:
      enum ShaderStyle {
        NOTEXTURE, FULLBRIGHT, SUNNY
      };
      MeshObject(const std::string &meshFile, const std::string &textureFile, glm::mat4 &transform, int style);
      MeshObject(const std::string &meshFile, glm::mat4 &transform);
      virtual ~MeshObject();

      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &proj, bool debug);
  };

  template <typename EcsInterface>
  MeshObject<EcsInterface>::MeshObject(const std::string &meshFile, const std::string &textureFile,
                                              glm::mat4 &transform, int style) : shaderStyle(style) {
    SCENE_ECS->addTransform(SCENE_ID, transform);

    // Load the mesh from file using assimp
    m_loadMesh(meshFile);
    // Load the texture from file using SDL2
    m_loadTexture(textureFile);
  }

  template <typename EcsInterface>
  MeshObject<EcsInterface>::MeshObject(const std::string &meshFile, glm::mat4 &transform) : shaderStyle(NOTEXTURE) {
    SCENE_ECS->addTransform(SCENE_ID, transform);

    // Load the mesh from file using assimp
    m_loadMesh(meshFile);
  }

  template <typename EcsInterface>
  MeshObject<EcsInterface>::~MeshObject() {
  }

  template <typename EcsInterface>
  void MeshObject<EcsInterface>::m_loadMesh(const std::string &meshFile) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile( meshFile, aiProcess_Triangulate );

    if (!scene) {
      fprintf(stderr, "Failed to load mesh from file: '%s'\n", meshFile.c_str());
    }
    if (scene->mNumMeshes != 1) {
      fprintf(stderr, "Mesh file '%s' must contain a single mesh", meshFile.c_str());
    }
    auto aim = scene->mMeshes[0];
    if (shaderStyle != FULLBRIGHT) {
      if (aim->mNormals == NULL) {
        fprintf(stderr, "Error: No normals in mesh '%s'\n", meshFile.c_str());
      }
    }
    if (shaderStyle != NOTEXTURE) {
      if (!aim->HasTextureCoords(0)) {
        fprintf(stderr, "Error: No texture coordinates in mesh '%s'\n", meshFile.c_str());
      }
    }

    void* pVertices = 0;
    size_t vertexSize = 0;
    switch(shaderStyle) {
      case FULLBRIGHT: {
        MeshVertex_NoNorm *vertices = new MeshVertex_NoNorm[aim->mNumVertices];
        for (size_t i = 0; i < aim->mNumVertices; ++i) {
          vertices[i].pos[0] = aim->mVertices[i].x;
          vertices[i].pos[1] = aim->mVertices[i].y;
          vertices[i].pos[2] = aim->mVertices[i].z;
          vertices[i].tex[0] = aim->mTextureCoords[0][i].x;
          vertices[i].tex[1] = aim->mTextureCoords[0][i].y;
        }
        pVertices = vertices;
        vertexSize = sizeof(MeshVertex_NoNorm);
      } break;
      case NOTEXTURE: {
        MeshVertex_NoTex *vertices = new MeshVertex_NoTex[aim->mNumVertices];
        for (size_t i = 0; i < aim->mNumVertices; ++i) {
          vertices[i].pos[0] = aim->mVertices[i].x;
          vertices[i].pos[1] = aim->mVertices[i].y;
          vertices[i].pos[2] = aim->mVertices[i].z;
          vertices[i].norm[0] = aim->mNormals[i].x;
          vertices[i].norm[1] = aim->mNormals[i].y;
          vertices[i].norm[2] = aim->mNormals[i].z;
        }
        pVertices = vertices;
        vertexSize = sizeof(MeshVertex_NoTex);
      } break;
      default: {
        MeshVertex *vertices = new MeshVertex[aim->mNumVertices];
        for (size_t i = 0; i < aim->mNumVertices; ++i) {
          vertices[i].pos[0] = aim->mVertices[i].x;
          vertices[i].pos[1] = aim->mVertices[i].y;
          vertices[i].pos[2] = aim->mVertices[i].z;
          vertices[i].norm[0] = aim->mNormals[i].x;
          vertices[i].norm[1] = aim->mNormals[i].y;
          vertices[i].norm[2] = aim->mNormals[i].z;
          vertices[i].tex[0] = aim->mTextureCoords[0][i].x;
          vertices[i].tex[1] = aim->mTextureCoords[0][i].y;
        }
        pVertices = vertices;
        vertexSize = sizeof(MeshVertex);
      }
    }

    // Copy the vertices buffer to the GL
    glGenBuffers(1, &m_vertexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    FORCE_ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,  // target
        vertexSize * aim->mNumVertices,  // size
        pVertices,  // data
        GL_STATIC_DRAW  // usage
    );
    FORCE_ASSERT_GL_ERROR();

    // Delete the local vertex information
    switch(shaderStyle) {
      case FULLBRIGHT: {
        delete[] (MeshVertex_NoNorm*) pVertices;
      } break;
      case NOTEXTURE: {
        delete[] (MeshVertex_NoTex*) pVertices;
      } break;
      default: {
        delete[] (MeshVertex*) pVertices;
      }
    }

    // Copy the face data into an index buffer
    uint32_t *indices = new uint32_t[3 * aim->mNumFaces];
    for (size_t i = 0; i < aim->mNumFaces; ++i) {
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

  template <typename EcsInterface>
  void MeshObject<EcsInterface>::m_loadTexture(const std::string &textureFile) {
    if (textureFile.empty()) {
      return;
    }
    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(textureFile.c_str(), &x, &y, &n, 0);
    if (data) {
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
    } else {
      fprintf(stderr, "Failed to load texture file '%s'.\n", textureFile.c_str());
    }
    stbi_image_free(data);
  }

  template <typename EcsInterface>
  void MeshObject<EcsInterface>::m_drawSurface(
      const glm::mat4 &model,
      const glm::mat4 &modelView,
      const glm::mat4 &projection)
  {
    // Use a simple shader
    std::shared_ptr<ShaderProgram> shader;
    switch (shaderStyle) {
      case SUNNY: {
        shader = Shaders::textureSunnyShader();
        shader->use();
      } break;
      case FULLBRIGHT: {
        shader = Shaders::textureFullbrightShader();
        shader->use();
      } break;
      default: { // NOTEXTURE
        shader = Shaders::noTextureSunnyShader();
        shader->use();
      } break;
    }

    // Prepare the uniform values
    if (shaderStyle != FULLBRIGHT) {
      assert(shader->modelLocation() != -1);
      glUniformMatrix4fv(
          shader->modelLocation(),  // location
          1,  // count
          0,  // transpose
          glm::value_ptr(model)  // value
      );
    }
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

    if (shaderStyle != NOTEXTURE) {
      // Prepare the texture sampler
      assert(shader->texture0() != -1);
      glUniform1i(
          shader->texture0(),  // location
          0  // value
      );                                                    ASSERT_GL_ERROR();
      glActiveTexture(GL_TEXTURE0);                         ASSERT_GL_ERROR();
      glBindTexture(GL_TEXTURE_2D, m_texture);              ASSERT_GL_ERROR();
    }

    // will re-use these for each attribute below...
    size_t stride = 0;
    void* pointer = 0;

    // Prepare the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    switch(shaderStyle) {
      case NOTEXTURE: {
        stride = sizeof(MeshVertex_NoTex);
        pointer = &(((MeshVertex_NoTex *) 0)->pos[0]);
      } break;
      case FULLBRIGHT: {
        stride = sizeof(MeshVertex_NoNorm);
        pointer = &(((MeshVertex_NoNorm *) 0)->pos[0]);
      } break;
      default: {
        stride = sizeof(MeshVertex);
        pointer = &(((MeshVertex *) 0)->pos[0]);
      }
    }
    glVertexAttribPointer(
        shader->vertPositionLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        stride,
        pointer
    );
    ASSERT_GL_ERROR();

    if (shaderStyle != FULLBRIGHT) {
      assert(shader->vertNormalLocation() != -1);
      glEnableVertexAttribArray(shader->vertNormalLocation());
      ASSERT_GL_ERROR();
      switch(shaderStyle) {
        case NOTEXTURE: {
          pointer = &(((MeshVertex_NoTex *) 0)->norm[0]);
        } break;
        default: {
          pointer = &(((MeshVertex *) 0)->norm[0]);
        }
      }
      glVertexAttribPointer(
          shader->vertNormalLocation(),  // index
          3,  // size
          GL_FLOAT,  // type
          0,  // normalized
          stride,
          pointer
      );
      ASSERT_GL_ERROR();
    }

    if (shaderStyle != NOTEXTURE) {
      assert(shader->vertTexCoordLocation() != -1);
      glEnableVertexAttribArray(shader->vertTexCoordLocation());
      ASSERT_GL_ERROR();
      switch(shaderStyle) {
        case FULLBRIGHT: {
          pointer = &(((MeshVertex_NoNorm *) 0)->tex[0]);
        } break;
        default: {
          pointer = &(((MeshVertex *) 0)->tex[0]);
        }
      }
      glVertexAttribPointer(
          shader->vertTexCoordLocation(),  // index
          2,  // size
          GL_FLOAT,  // type
          0,  // normalized
          stride,
          pointer
      );
      ASSERT_GL_ERROR();
    }

    // Draw the surface
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glDrawElements(
        GL_TRIANGLES,  // mode
        m_numIndices,  // count
        GL_UNSIGNED_INT,  // type
        0  // indices
    );
    ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void MeshObject<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView,
                                             const glm::mat4 &proj, bool debug)
  {
    glm::mat4 completeModelWorld = modelWorld;
    if (SCENE_ECS->hasTransformFunction(SCENE_ID)) {
      completeModelWorld *= SCENE_ECS->getTransformFunction(SCENE_ID);
    }
    glm::mat4 modelView = worldView * completeModelWorld;
    m_drawSurface(completeModelWorld, modelView, proj);
  }
}
