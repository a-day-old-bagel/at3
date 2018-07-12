/*
* Vulkan texture loader
*
* Copyright(C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * This code has been modified and added to by Galen Cochrane for use in the at3 project
 *
 * Additions to this code are also licensed under the MIT license as follows:
 *
 * Copyright(C) 2018 Galen Cochrane
 *
 * This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
 */

/*
 * TODO: Remove the standalone functions if similar functions already exist in the vkc code
 * TODO: Use resources in existing vkc code to better advantage
 * TODO: Make a repo of these things that works with the mesh repo
 * TODO: uncomment and fix the other classes
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <gli/gli.hpp>

#include "fileSystemHelpers.hpp"

namespace at3::vkc {

  /*
   * Get the index of a memory type that has all the requested property bits set
   */
  uint32_t chooseMemoryType(
      uint32_t typeBits, VkPhysicalDeviceMemoryProperties &availableProps, VkMemoryPropertyFlags desiredProps,
      VkBool32 *memTypeFound = nullptr);

  /*
   * Create an image memory barrier for changing the layout of
	 * an image and put it into an active command buffer
	 * See chapter 11.4 "Image Layout" for details
   */
  void setImageLayout(
      VkCommandBuffer cmdbuffer,
      VkImage image,
      VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout,
      VkImageSubresourceRange subresourceRange,
      VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  void setImageLayout(  // Calls the above function
      VkCommandBuffer cmdbuffer,
      VkImage image,
      VkImageAspectFlags aspectMask,
      VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout,
      VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  // Allocate and begin a new primary command buffer from a command pool
  VkCommandBuffer beginNewCommandBuffer(VkDevice device, VkCommandPool pool);

  void flushCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue);

  struct TextureOperationInfo {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkCommandPool transferCommandPool = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties physicalMemProps = {};
    VkBool32 samplerAnisotropy = VK_FALSE;
    float maxSamplerAnisotropy = 0;
    bool forceLinear = false;
    bool magFilterNearest = false;
  };

  struct Texture {
      VkImage image;
      VkImageLayout imageLayout;
      VkDeviceMemory deviceMemory;
      VkImageView view;
      uint32_t width, height;
      uint32_t mipLevels;
      uint32_t layerCount;
      VkSampler sampler;
      void destroy(TextureOperationInfo &info);
  };

  struct Texture2D {
    Texture texture;
    Texture2D (
        const std::string &filename,
        VkFormat format,
        TextureOperationInfo &info,
        VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  };

  class TextureRepository {
      std::vector<Texture2D> textures;
      std::vector<VkDescriptorImageInfo> descriptorImageInfos;
      std::unordered_map<std::string, uint32_t> textureArrayIndexMap;
      uint32_t nextId = 0;
      void registerNewTexture2D(Texture2D* texture2D, TextureOperationInfo &info, const std::string &key);
    public:
      TextureRepository(const std::string &textureDirectory, TextureOperationInfo &info);
      bool textureExists(const std::string &key);
      uint32_t getTextureArrayIndex(const std::string &key);
      VkDescriptorImageInfo* getDescriptorImageInfoArrayPtr();
      uint32_t getDescriptorImageInfoArrayCount();
  };






//  /** @brief Cube map texture */
//  class TextureCubeMap : public Texture {
//    public:
//      /**
//      * Load a cubemap texture including all mip levels from a single file
//      *
//      * @param filename File to load (supports .ktx and .dds)
//      * @param format Vulkan format of the image data stored in the file
//      * @param device Vulkan device to create the texture on
//      * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
//      * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
//      * @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
//      *
//      */
//      void loadFromFile(
//          std::string filename,
//          VkFormat format,
//          VulkanContext *ctxt,
//          VkQueue copyQueue,
//          VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
//          VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
//
//        if (this->ctxt) {
//          destroy();
//        }
//        this->ctxt = ctxt;
//
//        if (!vks::tools::fileExists(filename)) {
//          vks::tools::exitFatal("Could not load texture from " + filename +
//                                "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.",
//                                -1);
//        }
//        gli::texture_cube texCube(gli::load(filename));
//
//        assert(!texCube.empty());
//
//        this->device = device;
//        width = static_cast<uint32_t>(texCube.extent().x);
//        height = static_cast<uint32_t>(texCube.extent().y);
//        mipLevels = static_cast<uint32_t>(texCube.levels());
//
//        VkMemoryAllocateInfo memAllocInfo{};
//        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//
//        VkMemoryRequirements memReqs;
//
//        // Create a host-visible staging buffer that contains the raw image data
//        VkBuffer stagingBuffer;
//        VkDeviceMemory stagingMemory;
//
//        VkBufferCreateInfo bufferCreateInfo{};
//        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//
//        bufferCreateInfo.size = texCube.size();
//        // This buffer is used as a transfer source for the buffer copy
//        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//        VK_CHECK_RESULT(vkCreateBuffer(ctxt.logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
//
//        // Get memory requirements for the staging buffer (alignment, memory type bits)
//        vkGetBufferMemoryRequirements(ctxt.logicalDevice, stagingBuffer, &memReqs);
//
//        memAllocInfo.allocationSize = memReqs.size;
//        // Get memory type index for a host visible buffer
//        memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, ctxt.physicalMemProps,
//                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//
//        VK_CHECK_RESULT(vkAllocateMemory(ctxt.logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
//        VK_CHECK_RESULT(vkBindBufferMemory(ctxt.logicalDevice, stagingBuffer, stagingMemory, 0));
//
//        // Copy texture data into staging buffer
//        uint8_t *data;
//        VK_CHECK_RESULT(vkMapMemory(ctxt.logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **) &data));
//        memcpy(data, texCube.data(), texCube.size());
//        vkUnmapMemory(ctxt.logicalDevice, stagingMemory);
//
//        // Setup buffer copy regions for each face including all of it's miplevels
//        std::vector<VkBufferImageCopy> bufferCopyRegions;
//        size_t offset = 0;
//
//        for (uint32_t face = 0; face < 6; face++) {
//          for (uint32_t level = 0; level < mipLevels; level++) {
//            VkBufferImageCopy bufferCopyRegion = {};
//            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//            bufferCopyRegion.imageSubresource.mipLevel = level;
//            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
//            bufferCopyRegion.imageSubresource.layerCount = 1;
//            bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texCube[face][level].extent().x);
//            bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texCube[face][level].extent().y);
//            bufferCopyRegion.imageExtent.depth = 1;
//            bufferCopyRegion.bufferOffset = offset;
//
//            bufferCopyRegions.push_back(bufferCopyRegion);
//
//            // Increase offset into staging buffer for next level / face
//            offset += texCube[face][level].size();
//          }
//        }
//
//        // Create optimal tiled target image
//        VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
//        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//        imageCreateInfo.format = format;
//        imageCreateInfo.mipLevels = mipLevels;
//        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        imageCreateInfo.extent = {width, height, 1};
//        imageCreateInfo.usage = imageUsageFlags;
//        // Ensure that the TRANSFER_DST bit is set for staging
//        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
//          imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//        }
//        // Cube faces count as array layers in Vulkan
//        imageCreateInfo.arrayLayers = 6;
//        // This flag is required for cube map images
//        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
//
//
//        VK_CHECK_RESULT(vkCreateImage(ctxt.logicalDevice, &imageCreateInfo, nullptr, &image));
//
//        vkGetImageMemoryRequirements(ctxt.logicalDevice, image, &memReqs);
//
//        memAllocInfo.allocationSize = memReqs.size;
//        memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, ctxt.physicalMemProps,
//                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//        VK_CHECK_RESULT(vkAllocateMemory(ctxt.logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
//        VK_CHECK_RESULT(vkBindImageMemory(ctxt.logicalDevice, image, deviceMemory, 0));
//
//        // Use a separate command buffer for texture loading
//        VkCommandBuffer copyCmd;
//        ctxt->createCommandBuffer(copyCmd, ctxt.transferCommandPool); // TODO: Correct to use this pool?
//        VkCommandBufferBeginInfo cmdBufferBeginInfo{};
//        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//
//        // Image barrier for optimal image (target)
//        // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
//        VkImageSubresourceRange subresourceRange = {};
//        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        subresourceRange.baseMipLevel = 0;
//        subresourceRange.levelCount = mipLevels;
//        subresourceRange.layerCount = 6;
//
//        vks::tools::setImageLayout(
//            copyCmd,
//            image,
//            VK_IMAGE_LAYOUT_UNDEFINED,
//            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            subresourceRange);
//
//        // Copy the cube map faces from the staging buffer to the optimal tiled image
//        vkCmdCopyBufferToImage(
//            copyCmd,
//            stagingBuffer,
//            image,
//            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            static_cast<uint32_t>(bufferCopyRegions.size()),
//            bufferCopyRegions.data());
//
//        // Change texture image layout to shader read after all faces have been copied
//        this->imageLayout = imageLayout;
//        vks::tools::setImageLayout(
//            copyCmd,
//            image,
//            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            imageLayout,
//            subresourceRange);
//
//        device->flushCommandBuffer(copyCmd, copyQueue);
//
//        // Create sampler
//        VkSamplerCreateInfo samplerCreateInfo = vks::initializers::samplerCreateInfo();
//        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
//        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
//        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
//        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
//        samplerCreateInfo.mipLodBias = 0.0f;
//        samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy
//                                          ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
//        samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
//        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
//        samplerCreateInfo.minLod = 0.0f;
//        samplerCreateInfo.maxLod = (float) mipLevels;
//        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
//        VK_CHECK_RESULT(vkCreateSampler(ctxt.logicalDevice, &samplerCreateInfo, nullptr, &sampler));
//
//        // Create image view
//        VkImageViewCreateInfo viewCreateInfo = vks::initializers::imageViewCreateInfo();
//        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
//        viewCreateInfo.format = format;
//        viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
//                                     VK_COMPONENT_SWIZZLE_A};
//        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
//        viewCreateInfo.subresourceRange.layerCount = 6;
//        viewCreateInfo.subresourceRange.levelCount = mipLevels;
//        viewCreateInfo.image = image;
//        VK_CHECK_RESULT(vkCreateImageView(ctxt.logicalDevice, &viewCreateInfo, nullptr, &view));
//
//        // Clean up staging resources
//        vkFreeMemory(ctxt.logicalDevice, stagingMemory, nullptr);
//        vkDestroyBuffer(ctxt.logicalDevice, stagingBuffer, nullptr);
//
//        // Update descriptor image info member that can be used for setting up descriptor sets
//        updateDescriptor();
//      }
//  };

}
