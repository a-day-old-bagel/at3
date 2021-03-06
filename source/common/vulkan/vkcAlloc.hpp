#pragma once

#include <cstdint>
#include <cstdlib>

#include "vkcTypes.hpp"

namespace at3::vkc::passthrough {
  struct AllocatorState {
    size_t *memTypeAllocSizes;
    uint32_t totalAllocs;

    Common *context;
  };

  extern AllocatorState state;

  //ALLOCATOR INTERFACE / INSTALLATION
  void activate(Common *context);
  void alloc(Allocation &outAlloc, AllocationCreateInfo createInfo);
  void free(Allocation &handle);
  size_t allocatedSize(uint32_t memoryType);
  uint32_t numAllocs();

}

namespace at3::vkc::pool {
  struct OffsetSize {
    uint64_t offset;
    uint64_t size;
  };
  struct BlockSpanIndexPair {
    uint32_t blockIdx;
    uint32_t spanIdx;
  };

  struct DeviceMemoryBlock {
    Allocation mem;
    std::vector<OffsetSize> layout;
    bool pageReserved;
  };

  struct MemoryPool {
    std::vector<DeviceMemoryBlock> blocks;
  };

  struct AllocatorState {
    Common *context;

    std::vector<size_t> memTypeAllocSizes;
    uint32_t totalAllocs;

    uint32_t pageSize;
    VkDeviceSize memoryBlockMinSize;

    std::vector<MemoryPool> memPools;
  };

  extern AllocatorState state;

  //ALLOCATOR INTERFACE / INSTALLATION
  void activate(Common *context);
  void alloc(Allocation &outAlloc, AllocationCreateInfo createInfo);
  void free(Allocation &handle);
  size_t allocatedSize(uint32_t memoryType);
  uint32_t numAllocs();

}
