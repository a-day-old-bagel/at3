#pragma once

template<typename EcsInterface>
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext<EcsInterface>::debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t sourceObject,
    size_t location,
    int32_t messageCode,
    const char *layerPrefix,
    const char *message,
    void *userData) {
  std::ostringstream out;
  out << "VULKAN ";
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    out << "ERROR ";
  }
  if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    out << "WARNING ";
  }
  if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    out << "INFO ";
  }
  if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    out << "PERFORMANCE NOTE ";
  }
  if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
    out << "DEBUG ";
  }
  out << "(Layer " << layerPrefix << ", Code " << messageCode << "): " << message << std::endl;
  fprintf(stderr, "%s", out.str().c_str());
  fflush(stderr);

  return (VkBool32) false;
}

template<typename EcsInterface>
VkExtent2D VulkanContext<EcsInterface>::chooseSwapExtent() {
  VkSurfaceCapabilitiesKHR capabilities = common.gpu.swapChainSupport.capabilities;
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(common.windowWidth),
        static_cast<uint32_t>(common.windowHeight)
    };
    actualExtent.width = std::max(capabilities.minImageExtent.width,
                                  std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height,
                                   std::min(capabilities.maxImageExtent.height, actualExtent.height));
    return actualExtent;
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createSwapchainForSurface() {
  //choose the surface format to use
  VkSurfaceFormatKHR desiredFormat;
  VkPresentModeKHR desiredPresentMode;
  VkExtent2D swapExtent;

  VkcSwapChain &outSwapChain = common.swapChain;
  VkcPhysicalDevice &physDevice = common.gpu;
  const VkDevice &lDevice = common.device;
  const VkcSurface &surface = common.surface;

  bool foundFormat = false;

  //if there is no preferred format, the formats array only contains VK_FORMAT_UNDEFINED
  if (physDevice.swapChainSupport.formats.size() == 1 &&
      physDevice.swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED) {
    desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    desiredFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    foundFormat = true;
  }

  //otherwise we can't just choose any format we want, but still let's try to grab one that we know will work for us first
  if (!foundFormat) {
    for (uint32_t i = 0; i < physDevice.swapChainSupport.formats.size(); ++i) {
      const auto &availableFormat = physDevice.swapChainSupport.formats[i];

      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        desiredFormat = availableFormat;
      }
    }
  }

  //if our preferred format isn't available, let's just grab the first available because yolo
  if (!foundFormat) {
    desiredFormat = physDevice.swapChainSupport.formats[0];
  }

  // Prefer triple buffering, then immediate mode. Allow forced FIFO if user wants VSYNC.
  desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  if (!settings::graphics::vulkan::forceFifo) {
    for (const auto &availablePresentMode : physDevice.swapChainSupport.presentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        desiredPresentMode = availablePresentMode;
        printf("Using mailbox present mode (triple buffering).\n");
        break;
      } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        desiredPresentMode = availablePresentMode;
      }
    }
  }
  if (desiredPresentMode == VK_PRESENT_MODE_FIFO_KHR) {
    printf("Using FIFO present mode (double buffering).\n");
  } else if (desiredPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    printf("Using IMMEDIATE present mode.\n");
  }

  //update physdevice for new surface size
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice.device, surface.surface,
                                            &physDevice.swapChainSupport.capabilities);

  //swap extent is the resolution of the swapchain
  swapExtent = chooseSwapExtent();

  //need 1 more than minimum image count for triple buffering
  uint32_t imageCount = physDevice.swapChainSupport.capabilities.minImageCount + 1;
  if (physDevice.swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > physDevice.swapChainSupport.capabilities.maxImageCount) {
    imageCount = physDevice.swapChainSupport.capabilities.maxImageCount;
  }

  //now that everything is set up, we need to actually create the swap chain
  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = desiredFormat.format;
  createInfo.imageColorSpace = desiredFormat.colorSpace;
  createInfo.imageExtent = swapExtent;
  createInfo.imageArrayLayers = 1; //always 1 unless a stereoscopic app

  //here, we're rendering directly to the swap chain, but if we were using post processing, this might be VK_IMAGE_USAGE_TRANSFER_DST_BIT
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


  if (physDevice.graphicsQueueFamilyIdx != physDevice.presentQueueFamilyIdx) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    uint32_t queueFamilyIndices[] = {physDevice.graphicsQueueFamilyIdx, physDevice.presentQueueFamilyIdx};
    createInfo.pQueueFamilyIndices = queueFamilyIndices;

  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  createInfo.preTransform = physDevice.swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = desiredPresentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.pNext = NULL;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult res = vkCreateSwapchainKHR(lDevice, &createInfo, nullptr, &outSwapChain.swapChain);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating Vulkan Swapchain");

  //get images for swap chain
  vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, nullptr);
  outSwapChain.imageHandles.resize(imageCount);
  outSwapChain.imageViews.resize(imageCount);

  vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, &outSwapChain.imageHandles[0]);

  outSwapChain.imageFormat = desiredFormat.format;
  outSwapChain.extent = swapExtent;

  //create image views
  for (uint32_t i = 0; i < outSwapChain.imageHandles.size(); i++) {
    createImageView(outSwapChain.imageViews[i], outSwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                    outSwapChain.imageHandles[i]);
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::getWindowSize() {
  int width, height;
  SDL_GetWindowSize(common.window, &width, &height);
  if (width <= 0 || height <= 0) {
    return;
  }
  printf("Window size is %ux%u.\n", width, height);
  common.windowWidth = width;
  common.windowHeight = height;
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::createImageView(
    VkImageView &outView, VkFormat imageFormat, VkImageAspectFlags aspectMask,
    uint32_t mipCount, const VkImage &imageHdl) {
  VkImageViewCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = imageHdl;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = imageFormat;

  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  createInfo.subresourceRange.aspectMask = aspectMask;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = mipCount;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;


  VkResult res = vkCreateImageView(common.device, &createInfo, nullptr, &outView);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating Image View");

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::freeDeviceMemory(VkcAllocation &mem) {
  //this is sorta weird
  mem.context->allocator.free(mem);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createRenderPass(
    VkRenderPass &outPass, std::vector<VkAttachmentDescription> &colorAttachments,
    VkAttachmentDescription *depthAttachment) {
  std::vector<VkAttachmentReference> attachRefs;

  std::vector<VkAttachmentDescription> allAttachments;
  allAttachments = colorAttachments;

  uint32_t attachIdx = 0;
  while (attachIdx < colorAttachments.size()) {
    VkAttachmentReference ref = {0};
    ref.attachment = attachIdx++;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachRefs.push_back(ref);
  }

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
  subpass.pColorAttachments = &attachRefs[0];

  VkAttachmentReference depthRef = {0};

  if (depthAttachment) {
    depthRef.attachment = attachIdx;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depthRef;
    allAttachments.push_back(*depthAttachment);
  }


  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
  renderPassInfo.pAttachments = &allAttachments[0];
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  //we need a subpass dependency for transitioning the image to the right format, because by default, vulkan
  //will try to do that before we have acquired an image from our fb
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; //External means outside of the render pipeline, in srcPass, it means before the render pipeline
  dependency.dstSubpass = 0; //must be higher than srcSubpass
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //add the dependency to the renderpassinfo
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;


  VkResult res = vkCreateRenderPass(common.device, &renderPassInfo, nullptr, &outPass);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating render pass");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createCommandBuffer(VkCommandBuffer &outBuffer, VkCommandPool &pool) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkResult res = vkAllocateCommandBuffers(common.device, &allocInfo, &outBuffer);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating command buffer");
}

template<typename EcsInterface>
uint32_t VulkanContext<EcsInterface>::getMemoryType(
    const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
    VkMemoryPropertyFlags requiredProperties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

  //The VkPhysicalDeviceMemoryProperties structure has two arraysL memoryTypes and memoryHeaps.
  //Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM
  //for when VRAM runs out.The different types of memory exist within these heaps.Right now
  //we'll only concern ourselves with the type of memory and not the heap it comes from,
  //but you can imagine that this can affect performance.

  for (uint32_t memoryIndex = 0; memoryIndex < memProperties.memoryTypeCount; memoryIndex++) {
    const uint32_t memoryTypeBits = (1 << memoryIndex);
    const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

    const VkMemoryPropertyFlags properties = memProperties.memoryTypes[memoryIndex].propertyFlags;
    const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

    if (isRequiredMemoryType && hasRequiredProperties) {
      return static_cast<int32_t>(memoryIndex);
    }

  }

  AT3_ASSERT(0, "Could not find a valid memory type for requiredProperties");
  return 0;
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::createFrameBuffers(
    std::vector<VkFramebuffer> &outBuffers, const VkcSwapChain &swapChain, const VkImageView *depthBufferView,
    const VkRenderPass &renderPass) {
  outBuffers.resize(swapChain.imageViews.size());

  for (uint32_t i = 0; i < outBuffers.size(); i++) {
    std::vector<VkImageView> attachments;
    attachments.push_back(swapChain.imageViews[i]);

    if (depthBufferView) {
      attachments.push_back(*depthBufferView);
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = &attachments[0];
    framebufferInfo.width = swapChain.extent.width;
    framebufferInfo.height = swapChain.extent.height;
    framebufferInfo.layers = 1;

    VkResult r = vkCreateFramebuffer(common.device, &framebufferInfo, nullptr, &outBuffers[i]);
    AT3_ASSERT(r == VK_SUCCESS, "Error creating framebuffer");
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::allocateDeviceMemory(VkcAllocation &outMem, VkcAllocationCreateInfo info) {
  common.allocator.alloc(outMem, info);
}

template<typename EcsInterface>
VkcCommandBuffer VulkanContext<EcsInterface>::beginScratchCommandBuffer(VkcCmdPoolType type) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (type == VkcCmdPoolType::Graphics) {
    allocInfo.commandPool = common.gfxCommandPool;
  } else if (type == VkcCmdPoolType::Transfer) {
    allocInfo.commandPool = common.transferCommandPool;
  } else {
    allocInfo.commandPool = common.presentCommandPool;
  }

  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(common.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkcCommandBuffer outBuf;
  outBuf.buffer = commandBuffer;
  outBuf.owningPool = type;
  outBuf.context = &common;

  return outBuf;

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::submitScratchCommandBuffer(VkcCommandBuffer &commandBuffer) {
  vkEndCommandBuffer(commandBuffer.buffer);

  AT3_ASSERT(commandBuffer.context, "Attempting to submit a scratch command buffer that does not have a valid context");
  VkcCommon &common = *commandBuffer.context;

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer.buffer;

  VkQueue queue;
  VkCommandPool pool;
  if (commandBuffer.owningPool == VkcCmdPoolType::Graphics) {
    queue = common.deviceQueues.graphicsQueue;
    pool = common.gfxCommandPool;

  } else if (commandBuffer.owningPool == VkcCmdPoolType::Transfer) {
    queue = common.deviceQueues.transferQueue;
    pool = common.transferCommandPool;
  } else {
    queue = common.deviceQueues.presentQueue;
    pool = common.presentCommandPool;
  }


  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(common.device, pool, 1, &commandBuffer.buffer);
}


template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(
    VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
    uint32_t dstOffset, VkcCommandBuffer &buffer) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = srcOffset; // Optional
  copyRegion.dstOffset = dstOffset; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(buffer.buffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(
    VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
    uint32_t dstOffset, VkCommandBuffer &buffer) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = srcOffset; // Optional
  copyRegion.dstOffset = dstOffset; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(buffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(
    VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset,
    VkCommandBuffer *buffer) {
  if (!buffer) {
    VkcCommandBuffer scratch = beginScratchCommandBuffer(VkcCmdPoolType::Transfer);

    copyBuffer(srcBuffer, dstBuffer, size, srcOffset, dstOffset, scratch);

    submitScratchCommandBuffer(scratch);
  } else {
    copyBuffer(srcBuffer, dstBuffer, size, srcOffset, dstOffset, *buffer);
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createBuffer(
    VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  //concurrent so it can be used by the graphics and transfer queues

  std::vector<uint32_t> queues;
  queues.push_back(common.gpu.graphicsQueueFamilyIdx);

  if (common.gpu.graphicsQueueFamilyIdx != common.gpu.transferQueueFamilyIdx) {
    queues.push_back(common.gpu.transferQueueFamilyIdx);
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


  bufferInfo.pQueueFamilyIndices = &queues[0];
  bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

  VkResult res = vkCreateBuffer(common.device, &bufferInfo, nullptr, &outBuffer);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(common.device, outBuffer, &memRequirements);

  VkcAllocationCreateInfo allocInfo = {};
  allocInfo.size = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryType(common.gpu.device, memRequirements.memoryTypeBits, properties);
  allocInfo.usage = properties;

  common.allocator.alloc(bufferMemory, allocInfo);
  vkBindBufferMemory(common.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyDataToBuffer(
    VkBuffer *buffer, uint32_t dataSize, uint32_t dstOffset, char *data) {
  VkBuffer stagingBuffer;
  VkcAllocation stagingMemory;

  createBuffer(stagingBuffer, stagingMemory, dataSize,
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *mappedStagingBuffer;
  vkMapMemory(common.device, stagingMemory.handle, stagingMemory.offset, dataSize, 0, &mappedStagingBuffer);

  memset(mappedStagingBuffer, 0, dataSize);
  memcpy(mappedStagingBuffer, data, dataSize);

  vkUnmapMemory(common.device, stagingMemory.handle);

  VkcCommandBuffer scratch = beginScratchCommandBuffer(VkcCmdPoolType::Transfer);
  copyBuffer(stagingBuffer, *buffer, dataSize, 0, dstOffset, scratch);
  submitScratchCommandBuffer(scratch);

  vkDestroyBuffer(common.device, stagingBuffer, 0);
  freeDeviceMemory(stagingMemory);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createImage(
    VkImage &outImage, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult res = vkCreateImage(common.device, &imageInfo, nullptr, &outImage);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating vk image");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::copyBufferToImage(
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
  VkcCommandBuffer commandBuffer = beginScratchCommandBuffer(VkcCmdPoolType::Transfer);

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};


  vkCmdCopyBufferToImage(
      commandBuffer.buffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region
  );

  submitScratchCommandBuffer(commandBuffer);


}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::transitionImageLayout(
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkcCommandBuffer commandBuffer = beginScratchCommandBuffer(VkcCmdPoolType::Graphics);

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;

  //these are used to transfer queue ownership, which we aren't doing
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

  } else {
    //Unsupported layout transition
    AT3_ASSERT(0, "Attempting an unsupported image layout transition");
  }

  vkCmdPipelineBarrier(
      commandBuffer.buffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
  );


  submitScratchCommandBuffer(commandBuffer);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::allocMemoryForImage(
    VkcAllocation &outMem, const VkImage &image, VkMemoryPropertyFlags properties) {
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(common.device, image, &memRequirements);

  VkcAllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = getMemoryType(common.gpu.device, memRequirements.memoryTypeBits, properties);
  createInfo.usage = properties;
  allocateDeviceMemory(outMem, createInfo);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::initRendering(uint32_t num) {
  createMainRenderPass();
  createDepthBuffer();

  createFrameBuffers(common.renderData.frameBuffers, common.swapChain, &common.renderData.depthBuffer.view,
                     common.renderData.mainRenderPass);

  uint32_t swapChainImageCount = static_cast<uint32_t>(common.swapChain.imageViews.size());
  common.renderData.commandBuffers.resize(swapChainImageCount);
  for (uint32_t i = 0; i < swapChainImageCount; ++i) {
    createCommandBuffer(common.renderData.commandBuffers[i], common.gfxCommandPool);
  }
  loadUBOTestMaterial(common, num);

  common.renderData.firstFrame = std::vector<bool>(swapChainImageCount, true);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::updateDescriptorSets(DataStore *dataStore) {

  size_t oldNumPages = common.matData.descSets.size();
  size_t newNumPages = dataStore->getNumPages();

  common.matData.descSets.resize(newNumPages);

  for (size_t i = oldNumPages; i < newNumPages; ++i) {

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = common.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &common.matData.descSetLayout;

    VkResult res = vkAllocateDescriptorSets(common.device, &allocInfo, &common.matData.descSets[i]);
    AT3_ASSERT(res == VK_SUCCESS, "Error allocating global descriptor set");

    common.setWriters.clear();  // This *could* be faster than recreating a vector every update.

    common.bufferInfo = {};
    common.bufferInfo.buffer = dataStore->getPage(i);
    common.bufferInfo.offset = 0;
    common.bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet uboSetWriter = {};
    uboSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboSetWriter.dstBinding = 0;
    uboSetWriter.dstArrayElement = 0;
    uboSetWriter.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboSetWriter.descriptorCount = 1;
    uboSetWriter.dstSet = common.matData.descSets[i];
    uboSetWriter.pBufferInfo = &common.bufferInfo;
    uboSetWriter.pImageInfo = nullptr;
    common.setWriters.push_back(uboSetWriter);

    VkWriteDescriptorSet texSetWriter = {};
    texSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    texSetWriter.dstBinding = 1;
    texSetWriter.dstArrayElement = 0;
    texSetWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texSetWriter.descriptorCount = 1;
    texSetWriter.dstSet = common.matData.descSets[i];
    texSetWriter.pBufferInfo = nullptr;
    texSetWriter.pImageInfo = &testTex.descriptor;
    common.setWriters.push_back(texSetWriter);

    vkUpdateDescriptorSets(common.device, static_cast<uint32_t>(common.setWriters.size()), common.setWriters.data(), 0,
                           nullptr);
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createDepthBuffer() {
  createImage(common.renderData.depthBuffer.handle, common.swapChain.extent.width, common.swapChain.extent.height,
              VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(common.device, common.renderData.depthBuffer.handle, &memRequirements);

  VkcAllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = getMemoryType(common.gpu.device, memRequirements.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createInfo.usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  allocateDeviceMemory(common.renderData.depthBuffer.imageMemory, createInfo);

  vkBindImageMemory(common.device, common.renderData.depthBuffer.handle,
                    common.renderData.depthBuffer.imageMemory.handle,
                    common.renderData.depthBuffer.imageMemory.offset);

  createImageView(common.renderData.depthBuffer.view, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                  common.renderData.depthBuffer.handle);

  transitionImageLayout(common.renderData.depthBuffer.handle, VK_FORMAT_D32_SFLOAT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createMainRenderPass() {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = common.swapChain.imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


  std::vector<VkAttachmentDescription> renderPassAttachments;
  renderPassAttachments.push_back(colorAttachment);

  createRenderPass(common.renderData.mainRenderPass, renderPassAttachments, &depthAttachment);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::cleanupRendering() {
  vkDestroyImageView(common.device, common.renderData.depthBuffer.view, nullptr);
  vkDestroyImage(common.device, common.renderData.depthBuffer.handle, nullptr);
  allocators::pool::free(common.renderData.depthBuffer.imageMemory);

  for (size_t i = 0; i < common.renderData.frameBuffers.size(); i++) {
    vkDestroyFramebuffer(common.device, common.renderData.frameBuffers[i], nullptr);
  }

  vkFreeCommandBuffers(common.device, common.gfxCommandPool,
                       static_cast<uint32_t>(common.renderData.commandBuffers.size()),
                       common.renderData.commandBuffers.data());

  vkDestroyPipeline(common.device, common.matData.graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(common.device, common.matData.pipelineLayout, nullptr);
  vkDestroyRenderPass(common.device, common.renderData.mainRenderPass, nullptr);

  for (size_t i = 0; i < common.swapChain.imageViews.size(); i++) {
    vkDestroyImageView(common.device, common.swapChain.imageViews[i], nullptr);
  }

  vkDestroySwapchainKHR(common.device, common.swapChain.swapChain, nullptr);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::render(
    DataStore *dataStore, const glm::mat4 &wvMat, const MeshRepository <EcsInterface> &meshAssets,
    EcsInterface *ecs) {

  glm::mat4 proj = glm::perspective(glm::radians(60.f), common.windowWidth / (float) common.windowHeight, 0.1f,
                                    10000.f);
  // reverse the y
  proj[1][1] *= -1;

#if !COPY_ON_MAIN_COMMANDBUFFER
//  ubo_store::updateBuffers(wvMat, proj, nullptr, common);
  dataStore->updateBuffers(wvMat, proj, nullptr, common, ecs, meshAssets);
#endif

  VkResult res;

  //acquire an image from the swap chain
  uint32_t imageIndex;

  res = vkAcquireNextImageKHR(common.device, common.swapChain.swapChain, UINT64_MAX,
                              common.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    rtu::topics::publish("window_resized");
    return;
  } else {
    AT3_ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image!");
  }

  vkWaitForFences(common.device, 1, &common.frameFences[imageIndex], VK_FALSE, 5000000000);
  vkResetFences(common.device, 1, &common.frameFences[imageIndex]);

  //record drawing
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr; // Optional

  vkResetCommandBuffer(common.renderData.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  res = vkBeginCommandBuffer(common.renderData.commandBuffers[imageIndex], &beginInfo);

#if COPY_ON_MAIN_COMMANDBUFFER
  dataStore->updateBuffers(view, proj, &common.renderData.commandBuffers[imageIndex], common);
#endif


  vkCmdResetQueryPool(common.renderData.commandBuffers[imageIndex], common.queryPool, 0, 10);
  vkCmdWriteTimestamp(common.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      common.queryPool, 0);


  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = common.renderData.mainRenderPass;
  renderPassInfo.framebuffer = common.renderData.frameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = common.swapChain.extent;

  VkClearValue clearColors[2];
  clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearColors[1].depthStencil.depth = 1.0f;
  clearColors[1].depthStencil.stencil = 0;

  renderPassInfo.clearValueCount = static_cast<uint32_t>(2);
  renderPassInfo.pClearValues = &clearColors[0];

  vkCmdBeginRenderPass(common.renderData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  int currentlyBound = -1;

  vkCmdBindPipeline(common.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                    common.matData.graphicsPipeline);

  for (auto pair : meshAssets) {
    for (auto mesh : pair.second) {
      for (auto instance : mesh.instances) {
        glm::uint32 uboSlot = instance.uboIdx >> 16u;
        glm::uint32 uboPage = instance.uboIdx & 0xFFFFu;

        if (currentlyBound != uboPage) {
          vkCmdBindDescriptorSets(common.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  common.matData.pipelineLayout, 0, 1, &common.matData.descSets[uboPage], 0, nullptr);
          currentlyBound = uboPage;
        }

        vkCmdPushConstants(
            common.renderData.commandBuffers[imageIndex],
            common.matData.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(glm::uint32),
            (void *) &uboSlot);

        VkBuffer vertexBuffers[] = {mesh.buffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(common.renderData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(common.renderData.commandBuffers[imageIndex], mesh.buffer, mesh.iOffset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(common.renderData.commandBuffers[imageIndex], static_cast<uint32_t>(mesh.iCount), 1, 0, 0,
                         0);
      }
    }
  }

  vkCmdEndRenderPass(common.renderData.commandBuffers[imageIndex]);
  vkCmdWriteTimestamp(common.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      common.queryPool, 1);

  res = vkEndCommandBuffer(common.renderData.commandBuffers[imageIndex]);
  AT3_ASSERT(res == VK_SUCCESS, "Error ending render pass");

  // submit

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //wait on writing colours to the buffer until the semaphore says the buffer is available
  VkSemaphore waitSemaphores[] = {common.imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;

  VkSemaphore signalSemaphores[] = {common.renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.pCommandBuffers = &common.renderData.commandBuffers[imageIndex];
  submitInfo.commandBufferCount = 1;

  res = vkQueueSubmit(common.deviceQueues.graphicsQueue, 1, &submitInfo, common.frameFences[imageIndex]);
  AT3_ASSERT(res == VK_SUCCESS, "Error submitting queue");

  //present

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {common.swapChain.swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional
  res = vkQueuePresentKHR(common.deviceQueues.transferQueue, &presentInfo);

#if PRINT_VK_FRAMETIMES
  //log performance data:

  uint32_t end = 0;
  uint32_t begin = 0;

  static int count = 0;
  static float totalTime = 0.0f;

  if (count++ > 1024)
  {
    printf("VK Render Time (avg of past 1024 frames): %f ms\n", totalTime / 1024.0f);
    count = 0;
    totalTime = 0;
  }
  float timestampFrequency = common.gpu.deviceProps.limits.timestampPeriod;

  // A "firstFrame" bool is easier than setting up more synchronization objects.
  if (common.renderData.firstFrame[imageIndex]) {
    common.renderData.firstFrame[imageIndex] = false;
  } else {
    vkGetQueryPoolResults(common.device, common.queryPool, 1, 1, sizeof(uint32_t), &end, 0, VK_QUERY_RESULT_WAIT_BIT);
    vkGetQueryPoolResults(common.device, common.queryPool, 0, 1, sizeof(uint32_t), &begin, 0, VK_QUERY_RESULT_WAIT_BIT);
  }
  uint32_t diff = end - begin;
  totalTime += (diff) / (float)1e6;
#endif

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    rtu::topics::publish("window_resized");
  } else {
    AT3_ASSERT(res == VK_SUCCESS, "failed to present swap chain image!");
  }
}
