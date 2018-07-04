/*
* Copyright(C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * This code has been modified and added to by Galen Cochrane for use in the at3 project
 *
 * Additions to this code are also licensed under the MIT license as follows:
 *
 * Copyright(C) 2018 by Galen Cochrane
 */

//#pragma once

//#include "vkc.h"
#include <vulkan/vulkan.h>
#include "vkcTextures.hpp"

namespace at3::vkc {

  /**
		* Get the index of a memory type that has all the requested property bits set
		*
		* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
		* @param properties Bitmask of properties for the memory type to request
		* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
		*
		* @return Index of the requested memory type, or the max value of uint32_t as an error value.
		*/
  uint32_t chooseMemoryType(
      uint32_t typeBits, VkPhysicalDeviceMemoryProperties &availableProps, VkMemoryPropertyFlags desiredProps,
      VkBool32 *memTypeFound) {

    for (uint32_t i = 0; i < availableProps.memoryTypeCount; i++) {
      if ((typeBits & 1) == 1) {
        if ((availableProps.memoryTypes[i].propertyFlags & desiredProps) == desiredProps) {
          if (memTypeFound) {
            *memTypeFound = true;
          }
          return i;
        }
      }
      typeBits >>= 1;
    }
    if (memTypeFound) {
      *memTypeFound = false;
      return 0;
    } else {
      AT3_ASSERT(false, "Could not find a matching memory type");
      return static_cast<uint32_t>(-1);
    }
  }

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
      VkPipelineStageFlags srcStageMask,
      VkPipelineStageFlags dstStageMask) {

    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
      default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout) {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask =
            imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0) {
          imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
      default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
  }

  void setImageLayout(  // Calls the above function
      VkCommandBuffer cmdbuffer,
      VkImage image,
      VkImageAspectFlags aspectMask,
      VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout,
      VkPipelineStageFlags srcStageMask,
      VkPipelineStageFlags dstStageMask) {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
  }

  /**
		* Allocate and begin a new primary command buffer from a command pool
		*
		* @param pool The pool to use to allocate the new command buffer
		*
		* @return A handle to the allocated command buffer
		*/
		// TODO: Replace this with the scratch command buffer stuff
  VkCommandBuffer beginNewCommandBuffer(VkDevice device, VkCommandPool pool) {

    VkCommandBuffer buff;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult res = vkAllocateCommandBuffers(device, &allocInfo, &buff);
    AT3_ASSERT(res == VK_SUCCESS, "Failed to allocate command buffer!\n");

    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(buff, &cmdBufferBeginInfo);

    return buff;
  }

  /**
  * Finish command buffer and deallocate the buffer (use with beginNewCommandBuffer for best results)
  *
  * @param device Device to use
  * @param pool The pool from which to deallocate the command buffer
  * @param commandBuffer Command buffer to flush
  * @param queue Queue to submit the command buffer to
  *
  * @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
  * @note Uses a fence to ensure command buffer has finished executing
  */
  void flushCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue) {
    if (commandBuffer == VK_NULL_HANDLE) {
      return;
    }

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkFence fence;
    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

    // Submit to the queue
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device, 1, &fence, VK_TRUE, 5000000000); // 5s

    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
  }

  void Texture::destroy(TextureOperationInfo &info) {
    vkDestroyImageView(info.logicalDevice, view, nullptr);
    vkDestroyImage(info.logicalDevice, image, nullptr);
    if (sampler) {
      vkDestroySampler(info.logicalDevice, sampler, nullptr);
    }
    vkFreeMemory(info.logicalDevice, deviceMemory, nullptr);
  }

  Texture2D::Texture2D(
      const std::string &filename,
      VkFormat format,
      TextureOperationInfo &info,
      VkImageUsageFlags imageUsageFlags,
      VkImageLayout imageLayout) {

    AT3_ASSERT(fileExists(filename), "Could not load texture from file: %s\n", filename.c_str());

    gli::texture2d tex2D(gli::load(filename.c_str()));

    AT3_ASSERT(!tex2D.empty(), "Texture loaded, but empty: %s\n", filename.c_str());

    texture.width = static_cast<uint32_t>(tex2D[0].extent().x);
    texture.height = static_cast<uint32_t>(tex2D[0].extent().y);
    texture.mipLevels = static_cast<uint32_t>(tex2D.levels());

    printf("Loading Texture: %s (%u x %u with %u mip levels).\n", filename.c_str(),
           texture.width, texture.height, texture.mipLevels);

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

      for (uint32_t i = 0; i < texture.mipLevels; i++) {
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
      imageCreateInfo.mipLevels = texture.mipLevels;
      imageCreateInfo.arrayLayers = 1;
      imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageCreateInfo.extent = {texture.width, texture.height, 1};
      imageCreateInfo.usage = imageUsageFlags;
      // Ensure that the TRANSFER_DST bit is set for staging
      if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      }
      vkCreateImage(info.logicalDevice, &imageCreateInfo, nullptr, &texture.image);

      vkGetImageMemoryRequirements(info.logicalDevice, texture.image, &memReqs);

      memAllocInfo.allocationSize = memReqs.size;

      memAllocInfo.memoryTypeIndex = chooseMemoryType(memReqs.memoryTypeBits, info.physicalMemProps,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkAllocateMemory(info.logicalDevice, &memAllocInfo, nullptr, &texture.deviceMemory);
      vkBindImageMemory(info.logicalDevice, texture.image, texture.deviceMemory, 0);

      VkImageSubresourceRange subresourceRange = {};
      subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subresourceRange.baseMipLevel = 0;
      subresourceRange.levelCount = texture.mipLevels;
      subresourceRange.layerCount = 1;

      // Image barrier for optimal image (target)
      // Optimal image will be used as destination for the copy
      setImageLayout(
          copyCmd,
          texture.image,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          subresourceRange);

      // Copy mip levels from staging buffer
      vkCmdCopyBufferToImage(
          copyCmd,
          stagingBuffer,
          texture.image,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          static_cast<uint32_t>(bufferCopyRegions.size()),
          bufferCopyRegions.data()
      );

      // Change texture image layout to shader read after all mip levels have been copied
      texture.imageLayout = imageLayout;
      setImageLayout(
          copyCmd,
          texture.image,
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
      imageCreateInfo.extent = {texture.width, texture.height, 1};
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
      texture.image = mappableImage;
      texture.deviceMemory = mappableMemory;

      // Setup image memory barrier
      setImageLayout(copyCmd, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

      flushCommandBuffer(info.logicalDevice, info.transferCommandPool, copyCmd, info.transferQueue);
    }

    // Create a defaultsampler
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = info.magFilterNearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0f;
    // Max level-of-detail should match mip level count
    samplerCreateInfo.maxLod = (useStaging) ? (float) texture.mipLevels : 0.0f;

    // Only enable anisotropic filtering if enabled on the devicec
    samplerCreateInfo.anisotropyEnable = info.samplerAnisotropy;
    samplerCreateInfo.maxAnisotropy = samplerCreateInfo.anisotropyEnable ?
                                      info.maxSamplerAnisotropy : 1.0f;

    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(info.logicalDevice, &samplerCreateInfo, nullptr, &texture.sampler);

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
    viewCreateInfo.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
    viewCreateInfo.image = texture.image;
    VkResult result = vkCreateImageView(info.logicalDevice, &viewCreateInfo, nullptr, &texture.view);
    AT3_ASSERT(result == VK_SUCCESS, "Failed to create image view for %s!\n", filename.c_str());
  }

  void TextureRepository::registerNewTexture2D(
      Texture2D* texture2D, TextureOperationInfo &info, const std::string &key) {
    textureArrayIndexMap.emplace(key, nextId++);
    VkDescriptorImageInfo imageInfo {};
    imageInfo.sampler = texture2D->texture.sampler;
    imageInfo.imageView = texture2D->texture.view;
    imageInfo.imageLayout = texture2D->texture.imageLayout;
    descriptorImageInfos.push_back(imageInfo);
  }
  TextureRepository::TextureRepository(const std::string &textureDirectory, TextureOperationInfo &info) {
    for (auto &path : fs::recursive_directory_iterator(textureDirectory)) {
      if (getFileExtOnly(path) == ".ktx") {
        printf("TEX: %s : %u\n", getFileNameOnly(path).c_str(), static_cast<unsigned>(getFileNameOnly(path).size()));
        info.magFilterNearest = getFileNameOnly(path).size() < 3; // FIXME: This is going to irk somebody at some point.
        textures.emplace_back(getFileNameRelative(path), VK_FORMAT_R8G8B8A8_UNORM, info);
        registerNewTexture2D(&textures.back(), info, getFileNameOnly(path));
      }
    }
  }
  bool TextureRepository::textureExists(const std::string &key) {
    return textureArrayIndexMap.count(key) > 0;
  }
  uint32_t TextureRepository::getTextureArrayIndex(const std::string &key) {
    return textureArrayIndexMap.at(key);
  }
  VkDescriptorImageInfo* TextureRepository::getDescriptorImageInfoArrayPtr() {
    return descriptorImageInfos.data();
  }
  uint32_t TextureRepository::getDescriptorImageInfoArrayCount() {
    return static_cast<uint32_t>(descriptorImageInfos.size());
  }

}
