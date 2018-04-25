
#include "vkcDataStore.h"

namespace at3 {

  uint32_t getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
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

  void createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkcCommon &ctxt) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    //concurrent so it can be used by the graphics and transfer queues

    std::vector<uint32_t> queues;
    queues.push_back(ctxt.gpu.graphicsQueueFamilyIdx);

    if (ctxt.gpu.graphicsQueueFamilyIdx != ctxt.gpu.transferQueueFamilyIdx) {
      queues.push_back(ctxt.gpu.transferQueueFamilyIdx);
      bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    } else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


    bufferInfo.pQueueFamilyIndices = &queues[0];
    bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

    VkResult res = vkCreateBuffer(ctxt.device, &bufferInfo, nullptr, &outBuffer);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctxt.device, outBuffer, &memRequirements);

    VkcAllocationCreateInfo allocInfo = {};
    allocInfo.size = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
    allocInfo.usage = properties;

    ctxt.allocator.alloc(bufferMemory, allocInfo);
    vkBindBufferMemory(ctxt.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
  }
}
