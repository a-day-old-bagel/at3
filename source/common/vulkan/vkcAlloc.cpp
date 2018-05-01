
#include "vkcAlloc.h"
#include "configuration.h"

//Simple Passthrough allocator -> sub allocators responsible for actually parcelling out memory

namespace at3::allocators::passthrough {

  AllocatorState state;
  VkcAllocatorInterface allocImpl = {activate, alloc, free, allocatedSize, numAllocs};

  void activate(VkcCommon *context) {
    context->allocator = allocImpl;
    state.context = context;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->gpu.device, &memProperties);

    state.memTypeAllocSizes = (size_t *) calloc(1, sizeof(size_t) * memProperties.memoryTypeCount);
  }

  void deactivate(VkcCommon *context) {
    ::free(state.memTypeAllocSizes);
  }

  //IMPLEMENTATION


  void alloc(VkcAllocation &outAlloc, VkcAllocationCreateInfo createInfo) {
    state.totalAllocs++;
    state.memTypeAllocSizes[createInfo.memoryTypeIndex] += createInfo.size;

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = createInfo.size;
    allocInfo.memoryTypeIndex = createInfo.memoryTypeIndex;

    VkResult res = vkAllocateMemory(state.context->device, &allocInfo, nullptr, &(outAlloc.handle));

    outAlloc.size = createInfo.size;
    outAlloc.type = createInfo.memoryTypeIndex;
    outAlloc.offset = 0;
    outAlloc.context = state.context;

    AT3_ASSERT(res != VK_ERROR_OUT_OF_DEVICE_MEMORY, "Out of device memory");
    AT3_ASSERT(res != VK_ERROR_TOO_MANY_OBJECTS, "Attempting to create too many allocations")
    AT3_ASSERT(res == VK_SUCCESS, "Error allocating memory in passthrough allocator");
  }

  void free(VkcAllocation &allocation) {
    state.totalAllocs--;
    state.memTypeAllocSizes[allocation.type] -= allocation.size;
    vkFreeMemory(state.context->device, (allocation.handle), nullptr);
  }

  size_t allocatedSize(uint32_t memoryType) {
    return state.memTypeAllocSizes[memoryType];
  }

  uint32_t numAllocs() {
    return state.totalAllocs;
  }
}

namespace at3::allocators::pool {

  AllocatorState state;
  VkcAllocatorInterface allocImpl = {activate, alloc, free, allocatedSize, numAllocs};

  void activate(VkcCommon *context) {
    context->allocator = allocImpl;
    state.context = context;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->gpu.device, &memProperties);

    state.memTypeAllocSizes.resize(memProperties.memoryTypeCount);
    state.memPools.resize(memProperties.memoryTypeCount);

    state.pageSize = 1024; //can't use context->gpu.deviceProps.limits.bufferImageGranularity because some AMD cards set this to 1
    state.memoryBlockMinSize = state.pageSize * 10;
  }

  uint32_t addBlockToPool(VkDeviceSize size, uint32_t memoryType, bool fitToAlloc) {
    VkDeviceSize newPoolSize = size * 2;
    newPoolSize = newPoolSize < state.memoryBlockMinSize ? state.memoryBlockMinSize : newPoolSize;

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = newPoolSize;
    allocInfo.memoryTypeIndex = memoryType;

    DeviceMemoryBlock newBlock = {};
    VkResult res = vkAllocateMemory(state.context->device, &allocInfo, nullptr, &newBlock.mem.handle);

    AT3_ASSERT(res != VK_ERROR_OUT_OF_DEVICE_MEMORY, "Out of device memory");
    AT3_ASSERT(res != VK_ERROR_TOO_MANY_OBJECTS, "Attempting to create too many allocations")
    AT3_ASSERT(res == VK_SUCCESS, "Error allocating memory in passthrough allocator");

    newBlock.mem.type = memoryType;
    newBlock.mem.size = newPoolSize;

    MemoryPool &pool = state.memPools[memoryType];
    pool.blocks.push_back(newBlock);

    pool.blocks[pool.blocks.size() - 1].layout.push_back({0, newPoolSize});

    state.totalAllocs++;

    return pool.blocks.size() - 1;
  }

  void markChunkOfMemoryBlockUsed(uint32_t memoryType, BlockSpanIndexPair indices, VkDeviceSize size) {
    MemoryPool &pool = state.memPools[memoryType];

    pool.blocks[indices.blockIdx].layout[indices.spanIdx].offset += size;
    pool.blocks[indices.blockIdx].layout[indices.spanIdx].size -= size;
  }

  bool findFreeChunkForAllocation(BlockSpanIndexPair &outIndexPair, uint32_t memoryType, VkDeviceSize size,
                                  bool needsWholePage) {
    MemoryPool &pool = state.memPools[memoryType];

    for (uint32_t i = 0; i < pool.blocks.size(); ++i) {
      for (uint32_t j = 0; j < pool.blocks[i].layout.size(); ++j) {
        bool validOffset = needsWholePage ? pool.blocks[i].layout[j].offset == 0 : true;
        if (pool.blocks[i].layout[j].size >= size && validOffset) {
          outIndexPair.blockIdx = i;
          outIndexPair.spanIdx = j;
          return true;
        }
      }
    }
    return false;
  }

  void alloc(VkcAllocation &outAlloc, VkcAllocationCreateInfo createInfo) {
    uint32_t memoryType = createInfo.memoryTypeIndex;
    VkDeviceSize size = createInfo.size;

    MemoryPool &pool = state.memPools[memoryType];
    //make sure we always alloc a multiple of pageSize
    VkDeviceSize requestedAllocSize = ((size / state.pageSize) + 1) * state.pageSize;
    state.memTypeAllocSizes[memoryType] += requestedAllocSize;

    BlockSpanIndexPair location;

    bool needsOwnPage = createInfo.usage != VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bool found = findFreeChunkForAllocation(location, memoryType, requestedAllocSize, needsOwnPage);

    if (!found) {
      location = {addBlockToPool(requestedAllocSize, memoryType, needsOwnPage), 0};
    }

    pool.blocks[location.blockIdx].pageReserved = needsOwnPage;

    outAlloc.handle = pool.blocks[location.blockIdx].mem.handle;
    outAlloc.size = size;
    outAlloc.offset = pool.blocks[location.blockIdx].layout[location.spanIdx].offset;
    outAlloc.type = memoryType;
    outAlloc.id = location.blockIdx;
    outAlloc.context = state.context;

    markChunkOfMemoryBlockUsed(memoryType, location, requestedAllocSize);
  }

  void free(VkcAllocation &allocation) {
    VkDeviceSize requestedAllocSize = ((allocation.size / state.pageSize) + 1) * state.pageSize;

    OffsetSize span = {allocation.offset, requestedAllocSize};

    MemoryPool &pool = state.memPools[allocation.type];
    pool.blocks[allocation.id].pageReserved = false;

    bool found = false;

    uint32_t numLayoutMems = pool.blocks[allocation.id].layout.size();
    for (uint32_t j = 0; j < numLayoutMems; ++j) {
      if (pool.blocks[allocation.id].layout[j].offset == requestedAllocSize + allocation.offset) {
        pool.blocks[allocation.id].layout[j].offset = allocation.offset;
        pool.blocks[allocation.id].layout[j].size += requestedAllocSize;
        found = true;
        break;
      }
    }

    if (!found) {
      state.memPools[allocation.type].blocks[allocation.id].layout.push_back(span);
      state.memTypeAllocSizes[allocation.type] -= requestedAllocSize;
    }
  }

  size_t allocatedSize(uint32_t memoryType) {
    return state.memTypeAllocSizes[memoryType];
  }

  uint32_t numAllocs() {
    return state.totalAllocs;
  }

  void deactivate(VkcCommon *context) {
  }

}
