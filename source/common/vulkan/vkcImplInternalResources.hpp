#pragma once

template<typename EcsInterface>
uint32_t VulkanContext<EcsInterface>::make(
    MeshResource <EcsInterface> &outAsset, float *vertices, uint32_t vertexCount,
    uint32_t *indices, uint32_t indexCount) {
  size_t vBufferSize = vertexRenderData()->vertexSize * vertexCount;
  size_t iBufferSize = sizeof(uint32_t) * indexCount;

  MeshResource<EcsInterface> &m = outAsset;
  m.iCount = indexCount;
  m.vCount = vertexCount;

  createBuffer(m.buffer, m.bufferMemory, vBufferSize + iBufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //transfer data to the above buffers
  VkBuffer stagingBuffer;
  Allocation stagingMemory;

  createBuffer(stagingBuffer, stagingMemory, vBufferSize + iBufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;

  vkMapMemory(common.device, stagingMemory.handle, stagingMemory.offset, vBufferSize, 0, &data);
  memcpy(data, vertices, (size_t) vBufferSize);
  vkUnmapMemory(common.device, stagingMemory.handle);

  vkMapMemory(common.device, stagingMemory.handle, stagingMemory.offset + vBufferSize, iBufferSize, 0, &data);
  memcpy(data, indices, (size_t) iBufferSize);
  vkUnmapMemory(common.device, stagingMemory.handle);

  //copy to device local here
  copyBuffer(stagingBuffer, m.buffer, vBufferSize + iBufferSize, 0, 0, nullptr);
  freeDeviceMemory(stagingMemory);
  vkDestroyBuffer(common.device, stagingBuffer, nullptr);

  m.vOffset = 0;
  m.iOffset = vBufferSize;

  return 0;

}

template<typename EcsInterface>
MeshResources <EcsInterface> VulkanContext<EcsInterface>::loadMesh(
    const char *filepath, bool combineSubMeshes) {

  std::vector<MeshResource<EcsInterface>> outMeshes;

  const VertexRenderData *globalVertLayout = vertexRenderData();

  Assimp::Importer aiImporter;
  const struct aiScene *scene = NULL;

  struct aiLogStream stream;
  stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
  aiAttachLogStream(&stream);

  scene = aiImporter.ReadFile(filepath, MESH_FLAGS);

  const aiVector3D ZeroVector(0.0, 0.0, 0.0);
  const aiColor4D ZeroColor(0.0, 0.0, 0.0, 0.0);

  if (scene) {
    uint32_t floatsPerVert = globalVertLayout->vertexSize / sizeof(float);
    std::vector<float> vertexBuffer;
    std::vector<uint32_t> indexBuffer;
    uint32_t numVerts = 0;
    uint32_t numFaces = 0;

    outMeshes.resize(combineSubMeshes ? 1 : scene->mNumMeshes);

    for (uint32_t mIdx = 0; mIdx < scene->mNumMeshes; mIdx++) {
      if (!combineSubMeshes) {
        vertexBuffer.clear();
        indexBuffer.clear();
        numVerts = 0;
        numFaces = 0;
      }

      const aiMesh *mesh = scene->mMeshes[mIdx];

      for (uint32_t vIdx = 0; vIdx < mesh->mNumVertices; ++vIdx) {

        const aiVector3D *pos = &(mesh->mVertices[vIdx]);
        const aiVector3D *nrm = &(mesh->mNormals[vIdx]);
        const aiVector3D *uv0 = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][vIdx]) : &ZeroVector;
        const aiVector3D *uv1 = mesh->HasTextureCoords(1) ? &(mesh->mTextureCoords[1][vIdx]) : &ZeroVector;
        const aiVector3D *tan = mesh->HasTangentsAndBitangents() ? &(mesh->mTangents[vIdx]) : &ZeroVector;
        const aiVector3D *biTan = mesh->HasTangentsAndBitangents() ? &(mesh->mBitangents[vIdx]) : &ZeroVector;
        const aiColor4D *col = mesh->HasVertexColors(0) ? &(mesh->mColors[0][vIdx]) : &ZeroColor;

        for (uint32_t lIdx = 0; lIdx < globalVertLayout->attrCount; ++lIdx) {
          EMeshVertexAttribute comp = globalVertLayout->attributes[lIdx];

          switch (comp) {
            case EMeshVertexAttribute::POSITION: {
              vertexBuffer.push_back(pos->x);
              vertexBuffer.push_back(pos->y);
              vertexBuffer.push_back(pos->z);
            };
              break;
            case EMeshVertexAttribute::NORMAL: {
              vertexBuffer.push_back(nrm->x);
              vertexBuffer.push_back(nrm->y);
              vertexBuffer.push_back(nrm->z);
            };
              break;
            case EMeshVertexAttribute::UV0: {
              vertexBuffer.push_back(uv0->x);
              vertexBuffer.push_back(uv0->y);
            };
              break;
            case EMeshVertexAttribute::UV1: {
              vertexBuffer.push_back(uv1->x);
              vertexBuffer.push_back(uv1->y);
            };
              break;
            case EMeshVertexAttribute::TANGENT: {
              vertexBuffer.push_back(tan->x);
              vertexBuffer.push_back(tan->y);
              vertexBuffer.push_back(tan->z);
            };
              break;
            case EMeshVertexAttribute::BITANGENT: {
              vertexBuffer.push_back(biTan->x);
              vertexBuffer.push_back(biTan->y);
              vertexBuffer.push_back(biTan->z);
            };
              break;

            case EMeshVertexAttribute::COLOR: {
              vertexBuffer.push_back(col->r);
              vertexBuffer.push_back(col->g);
              vertexBuffer.push_back(col->b);
              vertexBuffer.push_back(col->a);
            };
              break;
          }

        }
      }

      for (unsigned int fIdx = 0; fIdx < mesh->mNumFaces; fIdx++) {
        const aiFace &face = mesh->mFaces[fIdx];
        AT3_ASSERT(face.mNumIndices == 3, "unsupported number of indices in mesh face");

        indexBuffer.push_back(numVerts + face.mIndices[0]);
        indexBuffer.push_back(numVerts + face.mIndices[1]);
        indexBuffer.push_back(numVerts + face.mIndices[2]);
      }

      numVerts += mesh->mNumVertices;
      numFaces += mesh->mNumFaces;

      if (!combineSubMeshes) {
        make(outMeshes[mIdx], vertexBuffer.data(), numVerts, indexBuffer.data(), indexBuffer.size());
      }
    }

    if (combineSubMeshes) {
      make(outMeshes[0], vertexBuffer.data(), numVerts, indexBuffer.data(), indexBuffer.size());
    }
  }

  aiDetachAllLogStreams();

  return outMeshes;
}






//template<typename EcsInterface>
//void
//VulkanContext<EcsInterface>::quad(
//    MeshResource <EcsInterface> &outAsset, float width, float height, float xOffset, float yOffset) {
//  const VertexRenderData *vertexData = vertexRenderData();
//
//  std::vector<float> verts;
//
//  float wComp = width / 2.0f;
//  float hComp = height / 2.0f;
//
//  const glm::vec3 lbCorner = glm::vec3(-wComp + xOffset, -hComp + yOffset, 0.0f);
//  const glm::vec3 ltCorner = glm::vec3(lbCorner.x, hComp + yOffset, 0.0f);
//  const glm::vec3 rbCorner = glm::vec3(wComp + xOffset, lbCorner.y, 0.0f);
//  const glm::vec3 rtCorner = glm::vec3(rbCorner.x, ltCorner.y, 0.0f);
//
//  const glm::vec3 pos[4] = {rtCorner, ltCorner, lbCorner, rbCorner};
//  const glm::vec2 uv[4] = {glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f),
//                           glm::vec2(1.0f, 0.0f)};
//
//
//  static uint32_t indices[6] = {0, 2, 1, 2, 0, 3};
//  uint32_t curIdx = 0;
//
//  for (uint32_t i = 0; i < 4; ++i) {
//
//    for (uint32_t j = 0; j < vertexData->attrCount; ++j) {
//      EMeshVertexAttribute attrib = vertexData->attributes[j];
//
//      switch (attrib) {
//        case EMeshVertexAttribute::POSITION: {
//          verts.push_back(pos[i].x);
//          verts.push_back(pos[i].y);
//          verts.push_back(pos[i].z);
//        }
//          break;
//        case EMeshVertexAttribute::NORMAL:
//        case EMeshVertexAttribute::TANGENT:
//        case EMeshVertexAttribute::BITANGENT: {
//          verts.push_back(0);
//          verts.push_back(0);
//          verts.push_back(0);
//        }
//          break;
//
//        case EMeshVertexAttribute::UV0: {
//          verts.push_back(uv[i].x);
//          verts.push_back(uv[i].y);
//        }
//          break;
//        case EMeshVertexAttribute::UV1: {
//          verts.push_back(0);
//          verts.push_back(0);
//        }
//          break;
//        case EMeshVertexAttribute::COLOR: {
//          verts.push_back(0);
//          verts.push_back(0);
//          verts.push_back(0);
//          verts.push_back(0);
//
//        }
//          break;
//        default:
//          AT3_ASSERT(0, "Invalid vertex attribute specified");
//          break;
//
//      }
//    }
//  }
//  make(outAsset, verts.data(), 4, &indices[0], 6);
//}
//
//
