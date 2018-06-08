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

#include "utilities.h"

namespace at3 {

  /**
		* Get the index of a memory type that has all the requested property bits set
		*/
  uint32_t chooseMemoryType(
      uint32_t typeBits, VkPhysicalDeviceMemoryProperties &availableProps, VkMemoryPropertyFlags desiredProps,
      VkBool32 *memTypeFound = nullptr);

  /**
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

  /**
  * Allocate and begin a new primary command buffer from a command pool
  */
  VkCommandBuffer beginNewCommandBuffer(VkDevice device, VkCommandPool pool);

  /**
  * Finish command buffer and deallocate the buffer (use with beginNewCommandBuffer for best results)
  */
  void flushCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue);

  /** @brief Info used to load textures without depending on VulkanContext wrapper (or any other wrapper) */
  struct VkcTextureOperationInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkCommandPool transferCommandPool;
    VkQueue transferQueue;
    VkPhysicalDeviceMemoryProperties physicalMemProps;
    VkBool32 samplerAnisotropy;
    float maxSamplerAnisotropy;
    bool forceLinear = false;
  };

  /** @brief Vulkan texture base class */
  template <typename EcsInterface>
  struct Texture {

      VkImage image;
      VkImageLayout imageLayout;
      VkDeviceMemory deviceMemory;
      VkImageView view;
      uint32_t width, height;
      uint32_t mipLevels;
      uint32_t layerCount;

      /** @brief Optional sampler to use with this texture */
      VkSampler sampler;

      /** @brief Release all Vulkan resources held by this texture */
      void destroy(VkcTextureOperationInfo &info) {
        vkDestroyImageView(info.logicalDevice, view, nullptr);
        vkDestroyImage(info.logicalDevice, image, nullptr);
        if (sampler) {
          vkDestroySampler(info.logicalDevice, sampler, nullptr);
        }
        vkFreeMemory(info.logicalDevice, deviceMemory, nullptr);
      }
  };

  /** @brief 2D texture */
  template <typename EcsInterface>
  struct Texture2D : public Texture<EcsInterface> {

      /**
      * Load a 2D texture including all mip levels
      */
      void loadFromFile(
          const std::string &filename,
          VkFormat format,
          VkcTextureOperationInfo &info,
          VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
          VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        AT3_ASSERT(fileExists(filename), "Could not load texture from file: %s\n", filename.c_str());

        gli::texture2d tex2D(gli::load(filename.c_str()));

        AT3_ASSERT(!tex2D.empty(), "Texture loaded, but empty: %s\n", filename.c_str());

        this->width = static_cast<uint32_t>(tex2D[0].extent().x);
        this->height = static_cast<uint32_t>(tex2D[0].extent().y);
        this->mipLevels = static_cast<uint32_t>(tex2D.levels());

        printf("Loading Texture: %s (%u x %u with %u mip levels).\n", filename.c_str(),
               this->width, this->height, this->mipLevels);

        // Get device properites for the requested texture format
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(info.physicalDevice, format, &formatProperties);

        // Only use linear tiling if requested (and supported by the device)
        // Support for linear tiling is mostly limited, so prefer to use
        // optimal tiling instead
        // On most implementations linear tiling will only support a very
        // limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
        auto useStaging = (VkBool32) !info.forceLinear;

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        VkMemoryRequirements memReqs;

        // Use a separate command buffer for texture loading TODO: Correct to use this pool?
        VkCommandBuffer copyCmd = beginNewCommandBuffer(info.logicalDevice, info.transferCommandPool);

        if (useStaging) {
          // Create a host-visible staging buffer that contains the raw image data
          VkBuffer stagingBuffer;
          VkDeviceMemory stagingMemory;

          VkBufferCreateInfo bufferCreateInfo{};
          bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

          bufferCreateInfo.size = tex2D.size();
          // This buffer is used as a transfer source for the buffer copy
          bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
          bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

          vkCreateBuffer(info.logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

          // Get memory requirements for the staging buffer (alignment, memory type bits)
          vkGetBufferMemoryRequirements(info.logicalDevice, stagingBuffer, &memReqs);

          memAllocInfo.allocationSize = memReqs.size;

          // Get memory type index for a host visible buffer
          memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, info.physicalMemProps,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

          vkAllocateMemory(info.logicalDevice, &memAllocInfo, nullptr, &stagingMemory);
          vkBindBufferMemory(info.logicalDevice, stagingBuffer, stagingMemory, 0);

          // Copy texture data into staging buffer
          uint8_t *data;
          vkMapMemory(info.logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **) &data);
          memcpy(data, tex2D.data(), tex2D.size());
          vkUnmapMemory(info.logicalDevice, stagingMemory);

          // Setup buffer copy regions for each mip level
          std::vector<VkBufferImageCopy> bufferCopyRegions;
          uint32_t offset = 0;

          for (uint32_t i = 0; i < this->mipLevels; i++) {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = i;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2D[i].extent().x);
            bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2D[i].extent().y);
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            bufferCopyRegions.push_back(bufferCopyRegion);

            offset += static_cast<uint32_t>(tex2D[i].size());
          }

          // Create optimal tiled target image
          VkImageCreateInfo imageCreateInfo{};
          imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

          imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
          imageCreateInfo.format = format;
          imageCreateInfo.mipLevels = this->mipLevels;
          imageCreateInfo.arrayLayers = 1;
          imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
          imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
          imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          imageCreateInfo.extent = {this->width, this->height, 1};
          imageCreateInfo.usage = imageUsageFlags;
          // Ensure that the TRANSFER_DST bit is set for staging
          if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
          }
          vkCreateImage(info.logicalDevice, &imageCreateInfo, nullptr, &this->image);

          vkGetImageMemoryRequirements(info.logicalDevice, this->image, &memReqs);

          memAllocInfo.allocationSize = memReqs.size;

          memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, info.physicalMemProps,
                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
          vkAllocateMemory(info.logicalDevice, &memAllocInfo, nullptr, &this->deviceMemory);
          vkBindImageMemory(info.logicalDevice, this->image, this->deviceMemory, 0);

          VkImageSubresourceRange subresourceRange = {};
          subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          subresourceRange.baseMipLevel = 0;
          subresourceRange.levelCount = this->mipLevels;
          subresourceRange.layerCount = 1;

          // Image barrier for optimal image (target)
          // Optimal image will be used as destination for the copy
          setImageLayout(
              copyCmd,
              this->image,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              subresourceRange);

          // Copy mip levels from staging buffer
          vkCmdCopyBufferToImage(
              copyCmd,
              stagingBuffer,
              this->image,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              static_cast<uint32_t>(bufferCopyRegions.size()),
              bufferCopyRegions.data()
          );

          // Change texture image layout to shader read after all mip levels have been copied
          this->imageLayout = imageLayout;
          setImageLayout(
              copyCmd,
              this->image,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              imageLayout,
              subresourceRange);

          flushCommandBuffer(info.logicalDevice, info.transferCommandPool, copyCmd, info.transferQueue);

          // Clean up staging resources
          vkFreeMemory(info.logicalDevice, stagingMemory, nullptr);
          vkDestroyBuffer(info.logicalDevice, stagingBuffer, nullptr);
        } else {
          // Prefer using optimal tiling, as linear tiling
          // may support only a small set of features
          // depending on implementation (e.g. no mip maps, only one layer, etc.)

          // Check if this support is supported for linear tiling
          AT3_ASSERT(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                     "Linear tiling not supported!\n");

          VkImage mappableImage;
          VkDeviceMemory mappableMemory;

          VkImageCreateInfo imageCreateInfo{};
          imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

          imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
          imageCreateInfo.format = format;
          imageCreateInfo.extent = {this->width, this->height, 1};
          imageCreateInfo.mipLevels = 1;
          imageCreateInfo.arrayLayers = 1;
          imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
          imageCreateInfo.usage = imageUsageFlags;
          imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
          imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

          // Load mip map level 0 to linear tiling image
          vkCreateImage(info.logicalDevice, &imageCreateInfo, nullptr, &mappableImage);

          // Get memory requirements for this image
          // like size and alignment
          vkGetImageMemoryRequirements(info.logicalDevice, mappableImage, &memReqs);
          // Set memory allocation size to required memory size
          memAllocInfo.allocationSize = memReqs.size;

          // Get memory type that can be mapped to host memory
          memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, info.physicalMemProps,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

          // Allocate host memory
          vkAllocateMemory(info.logicalDevice, &memAllocInfo, nullptr, &mappableMemory);

          // Bind allocated image for use
          vkBindImageMemory(info.logicalDevice, mappableImage, mappableMemory, 0);

          // Get sub resource layout
          // Mip map count, array layer, etc.
          VkImageSubresource subRes = {};
          subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          subRes.mipLevel = 0;

          VkSubresourceLayout subResLayout;
          void *data;

          // Get sub resources layout
          // Includes row pitch, size offsets, etc.
          vkGetImageSubresourceLayout(info.logicalDevice, mappableImage, &subRes, &subResLayout);

          // Map image memory
          vkMapMemory(info.logicalDevice, mappableMemory, 0, memReqs.size, 0, &data);

          // Copy image data into memory
          memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

          vkUnmapMemory(info.logicalDevice, mappableMemory);

          // Linear tiled images don't need to be staged
          // and can be directly used as textures
          this->image = mappableImage;
          this->deviceMemory = mappableMemory;

          // Setup image memory barrier
          setImageLayout(copyCmd, this->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

          flushCommandBuffer(info.logicalDevice, info.transferCommandPool, copyCmd, info.transferQueue);
        }

        // Create a defaultsampler
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        // Max level-of-detail should match mip level count
        samplerCreateInfo.maxLod = (useStaging) ? (float) this->mipLevels : 0.0f;

        // Only enable anisotropic filtering if enabled on the devicec
        samplerCreateInfo.anisotropyEnable = info.samplerAnisotropy;
        samplerCreateInfo.maxAnisotropy = samplerCreateInfo.anisotropyEnable ?
            info.maxSamplerAnisotropy : 1.0f;

        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        vkCreateSampler(info.logicalDevice, &samplerCreateInfo, nullptr, &this->sampler);

        // Create image view
        // Textures are not directly accessed by the shaders and
        // are abstracted by image views containing additional
        // information and sub resource ranges
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                                     VK_COMPONENT_SWIZZLE_A};
        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        // Linear tiling usually won't support mip maps
        // Only set mip map count if optimal tiling is used
        viewCreateInfo.subresourceRange.levelCount = (useStaging) ? this->mipLevels : 1;
        viewCreateInfo.image = this->image;
        VkResult result = vkCreateImageView(info.logicalDevice, &viewCreateInfo, nullptr, &this->view);
        AT3_ASSERT(result == VK_SUCCESS, "Failed to create image view for %s!\n", filename.c_str());
      }
  };





//  template<typename EcsInterface>
//  std::unique_ptr<std::unordered_map<std::string, uint32_t>> Texture2D<EcsInterface>::textureArrayIndexMap;
//  template<typename EcsInterface>
//  std::unique_ptr<std::vector<VkDescriptorImageInfo>> Texture2D<EcsInterface>::descriptorImageInfos;
//  template<typename EcsInterface>
//  std::unique_ptr<uint32_t> Texture2D<EcsInterface>::nextId;



  template<typename EcsInterface>
  class TextureRepository {
      std::vector<Texture2D<EcsInterface>> textures;
      std::vector<VkDescriptorImageInfo> descriptorImageInfos;
      std::unordered_map<std::string, uint32_t> textureArrayIndexMap;
      uint32_t nextId = 0;
      void registerNewTexture2D(Texture2D<EcsInterface>* texture, VkcTextureOperationInfo &info,
                                       const std::string &key) {
        printf("Texture %u registered: %s\n", nextId, key.c_str());
        textureArrayIndexMap.emplace(key, nextId++);
        VkDescriptorImageInfo imageInfo {};
        imageInfo.sampler = texture->sampler;
        imageInfo.imageView = texture->view;
        imageInfo.imageLayout = texture->imageLayout;
        descriptorImageInfos.push_back(imageInfo);
      }
    public:
      TextureRepository(const std::string &textureDirectory, VkcTextureOperationInfo &info) {
        for (auto &path : fs::recursive_directory_iterator(textureDirectory)) {
          if (getFileExtOnly(path) == ".ktx") {
            textures.emplace_back();
            textures.back().loadFromFile(getFileNameRelative(path), VK_FORMAT_R8G8B8A8_UNORM, info);
            registerNewTexture2D(&textures.back(), info, getFileNameOnly(path));
          }
        }
      }
      bool textureExists(const std::string &key) {
        return textureArrayIndexMap.count(key) > 0;
      }
      uint32_t getTextureArrayIndex(const std::string &key) {
        return textureArrayIndexMap.at(key);
      }
      VkDescriptorImageInfo* getDescriptorImageInfoArrayPtr() {
        return descriptorImageInfos.data();
      }
      uint32_t getDescriptorImageInfoArrayCount() {
        return static_cast<uint32_t>(descriptorImageInfos.size());
      }
  };







//  /** @brief 2D array texture */
//  class Texture2DArray : public Texture {
//    public:
//      /**
//      * Load a 2D texture array including all mip levels
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
//        gli::texture2d_array tex2DArray(gli::load(filename));
//
//        assert(!tex2DArray.empty());
//
//        this->device = device;
//        width = static_cast<uint32_t>(tex2DArray.extent().x);
//        height = static_cast<uint32_t>(tex2DArray.extent().y);
//        layerCount = static_cast<uint32_t>(tex2DArray.layers());
//        mipLevels = static_cast<uint32_t>(tex2DArray.levels());
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
//        bufferCreateInfo.size = tex2DArray.size();
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
//        memcpy(data, tex2DArray.data(), static_cast<size_t>(tex2DArray.size()));
//        vkUnmapMemory(ctxt.logicalDevice, stagingMemory);
//
//        // Setup buffer copy regions for each layer including all of it's miplevels
//        std::vector<VkBufferImageCopy> bufferCopyRegions;
//        size_t offset = 0;
//
//        for (uint32_t layer = 0; layer < layerCount; layer++) {
//          for (uint32_t level = 0; level < mipLevels; level++) {
//            VkBufferImageCopy bufferCopyRegion = {};
//            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//            bufferCopyRegion.imageSubresource.mipLevel = level;
//            bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
//            bufferCopyRegion.imageSubresource.layerCount = 1;
//            bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2DArray[layer][level].extent().x);
//            bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2DArray[layer][level].extent().y);
//            bufferCopyRegion.imageExtent.depth = 1;
//            bufferCopyRegion.bufferOffset = offset;
//
//            bufferCopyRegions.push_back(bufferCopyRegion);
//
//            // Increase offset into staging buffer for next level / face
//            offset += tex2DArray[layer][level].size();
//          }
//        }
//
//        // Create optimal tiled target image
//        VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
//        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//        imageCreateInfo.format = format;
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
//        imageCreateInfo.arrayLayers = layerCount;
//        imageCreateInfo.mipLevels = mipLevels;
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
//        subresourceRange.layerCount = layerCount;
//
//        vks::tools::setImageLayout(
//            copyCmd,
//            image,
//            VK_IMAGE_LAYOUT_UNDEFINED,
//            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            subresourceRange);
//
//        // Copy the layers and mip levels from the staging buffer to the optimal tiled image
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
//        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
//        viewCreateInfo.format = format;
//        viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
//                                     VK_COMPONENT_SWIZZLE_A};
//        viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
//        viewCreateInfo.subresourceRange.layerCount = layerCount;
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
//
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
