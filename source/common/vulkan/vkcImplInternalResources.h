#pragma once

//// TODO: Get rid of this if new texture code matches - uses all existing code if possible
//template<typename EcsInterface>
//void VulkanContext<EcsInterface>::makeTexture(VkcTextureResource &outResource, const char *filepath) {
//  VkcTextureResource &t = outResource;
//
//  int texWidth, texHeight, texChannels;
//
//  //STBI_rgb_alpha forces an alpha even if the image doesn't have one
//  stbi_uc *pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//
//  AT3_ASSERT(pixels, "Could not load image");
//
//  VkDeviceSize imageSize = texWidth * texHeight * 4;
//
//  VkBuffer stagingBuffer;
//  VkcAllocation stagingBufferMemory;
//
//  createBuffer(stagingBuffer, stagingBufferMemory, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//
//  void *data;
//  vkMapMemory(common.device, stagingBufferMemory.handle, stagingBufferMemory.offset, imageSize, 0, &data);
//  memcpy(data, pixels, static_cast<size_t>(imageSize));
//  vkUnmapMemory(common.device, stagingBufferMemory.handle);
//
//  stbi_image_free(pixels);
//
//  t.width = texWidth;
//  t.height = texHeight;
//  t.numChannels = texChannels;
//  t.format = VK_FORMAT_R8G8B8A8_UNORM;
//
//  //VK image format must match buffer
//  createImage(t.image, t.width, t.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
//              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
//
//  allocMemoryForImage(t.deviceMemory, t.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//  vkBindImageMemory(common.device, t.image, t.deviceMemory.handle, t.deviceMemory.offset);
//
//  transitionImageLayout(t.image, t.format,
//                        VK_IMAGE_LAYOUT_UNDEFINED,
//                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//  copyBufferToImage(stagingBuffer, t.image, t.width, t.height);
//
//  transitionImageLayout(t.image, t.format,
//                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
//  createImageView(t.view, t.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, t.image);
//
//  vkDestroyBuffer(common.device, stagingBuffer, nullptr);
//  freeDeviceMemory(stagingBufferMemory);
//}

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
  VkcAllocation stagingMemory;

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






//template<typename EcsInterface>
//void VulkanContext<EcsInterface>::loadTextures() {
//
//  // Vulkan core supports three different compressed texture formats
//  // As the support differs between implemementations we need to check device features and select a proper format and file
//  std::string filename;
//  VkFormat format = VK_FORMAT_UNDEFINED;
//  if (common.gpu.features.textureCompressionBC) {
//    filename = "texturearray_bc3_unorm.ktx";
//    format = VK_FORMAT_BC3_UNORM_BLOCK;
//  } else if (common.gpu.features.textureCompressionASTC_LDR) {
//    filename = "texturearray_astc_8x8_unorm.ktx";
//    format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
//  } else if (common.gpu.features.textureCompressionETC2) {
//    filename = "texturearray_etc2_unorm.ktx";
//    format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
//  }
//  AT3_ASSERT(format != VK_FORMAT_UNDEFINED, "Device does not support any compressed texture format!");
//
//  loadTextureArray("assets/textures/" + filename, format);
//}
//
//template<typename EcsInterface>
//void VulkanContext<EcsInterface>::loadTextureArray(std::string filename, VkFormat format) {
//
//  gli::texture2d_array tex2DArray(gli::load(filename));
//
//  AT3_ASSERT(!tex2DArray.empty(), "Could not load textures for texture array!\n");
//
//  textureArray.width = tex2DArray.extent().x;
//  textureArray.height = tex2DArray.extent().y;
//  layerCount = tex2DArray.layers();
//
//  VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
//  VkMemoryRequirements memReqs;
//
//  // Create a host-visible staging buffer that contains the raw image data
//  VkBuffer stagingBuffer;
//  VkDeviceMemory stagingMemory;
//
//  VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
//  bufferCreateInfo.size = tex2DArray.size();
//  // This buffer is used as a transfer source for the buffer copy
//  bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//  VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
//
//  // Get memory requirements for the staging buffer (alignment, memory type bits)
//  vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
//
//  memAllocInfo.allocationSize = memReqs.size;
//  // Get memory type index for a host visible buffer
//  memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits,
//                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//
//  VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
//  VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));
//
//  // Copy texture data into staging buffer
//  uint8_t *data;
//  VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **) &data));
//  memcpy(data, tex2DArray.data(), tex2DArray.size());
//  vkUnmapMemory(device, stagingMemory);
//
//  // Setup buffer copy regions for array layers
//  std::vector<VkBufferImageCopy> bufferCopyRegions;
//  size_t offset = 0;
//
//  for (uint32_t layer = 0; layer < layerCount; layer++) {
//    VkBufferImageCopy bufferCopyRegion = {};
//    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    bufferCopyRegion.imageSubresource.mipLevel = 0;
//    bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
//    bufferCopyRegion.imageSubresource.layerCount = 1;
//    bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2DArray[layer][0].extent().x);
//    bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2DArray[layer][0].extent().y);
//    bufferCopyRegion.imageExtent.depth = 1;
//    bufferCopyRegion.bufferOffset = offset;
//
//    bufferCopyRegions.push_back(bufferCopyRegion);
//
//    // Increase offset into staging buffer for next level / face
//    offset += tex2DArray[layer][0].size();
//  }
//
//  // Create optimal tiled target image
//  VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
//  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//  imageCreateInfo.format = format;
//  imageCreateInfo.mipLevels = 1;
//  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//  imageCreateInfo.extent = {textureArray.width, textureArray.height, 1};
//  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//  imageCreateInfo.arrayLayers = layerCount;
//
//  VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &textureArray.image));
//
//  vkGetImageMemoryRequirements(device, textureArray.image, &memReqs);
//
//  memAllocInfo.allocationSize = memReqs.size;
//  memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits,
//                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//  VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &textureArray.deviceMemory));
//  VK_CHECK_RESULT(vkBindImageMemory(device, textureArray.image, textureArray.deviceMemory, 0));
//
//  VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//
//  // Image barrier for optimal image (target)
//  // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
//  VkImageSubresourceRange subresourceRange = {};
//  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//  subresourceRange.baseMipLevel = 0;
//  subresourceRange.levelCount = 1;
//  subresourceRange.layerCount = layerCount;
//
//  vks::tools::setImageLayout(
//      copyCmd,
//      textureArray.image,
//      VK_IMAGE_LAYOUT_UNDEFINED,
//      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//      subresourceRange);
//
//  // Copy the cube map faces from the staging buffer to the optimal tiled image
//  vkCmdCopyBufferToImage(
//      copyCmd,
//      stagingBuffer,
//      textureArray.image,
//      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//      bufferCopyRegions.size(),
//      bufferCopyRegions.data()
//  );
//
//  // Change texture image layout to shader read after all faces have been copied
//  textureArray.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//  vks::tools::setImageLayout(
//      copyCmd,
//      textureArray.image,
//      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//      textureArray.imageLayout,
//      subresourceRange);
//
//  VulkanExampleBase::flushCommandBuffer(copyCmd, queue, true);
//
//  // Create sampler
//  VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
//  sampler.magFilter = VK_FILTER_LINEAR;
//  sampler.minFilter = VK_FILTER_LINEAR;
//  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//  sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//  sampler.addressModeV = sampler.addressModeU;
//  sampler.addressModeW = sampler.addressModeU;
//  sampler.mipLodBias = 0.0f;
//  sampler.maxAnisotropy = 8;
//  sampler.compareOp = VK_COMPARE_OP_NEVER;
//  sampler.minLod = 0.0f;
//  sampler.maxLod = 0.0f;
//  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
//  VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &textureArray.sampler));
//
//  // Create image view
//  VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
//  view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
//  view.format = format;
//  view.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
//  view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
//  view.subresourceRange.layerCount = layerCount;
//  view.subresourceRange.levelCount = 1;
//  view.image = textureArray.image;
//  VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &textureArray.view));
//
//  // Clean up staging resources
//  vkFreeMemory(device, stagingMemory, nullptr);
//  vkDestroyBuffer(device, stagingBuffer, nullptr);
//}
