#pragma once

#include <cstdint>
#include <cstdlib>
//#include "vkh.h"

#include "vkcTypes.h"
#include "vkcInitializers.h"

namespace at3::allocators::passthrough {
  struct AllocatorState {
    size_t *memTypeAllocSizes;
    uint32_t totalAllocs;

    VkcCommon *context;
  };

  extern AllocatorState state;

  //ALLOCATOR INTERFACE / INSTALLATION
  void activate(VkcCommon *context);
  void alloc(VkcAllocation &outAlloc, VkcAllocationCreateInfo createInfo);
  void free(VkcAllocation &handle);
  size_t allocatedSize(uint32_t memoryType);
  uint32_t numAllocs();

}

namespace at3::allocators::pool {
  struct OffsetSize {
    uint64_t offset;
    uint64_t size;
  };
  struct BlockSpanIndexPair {
    uint32_t blockIdx;
    uint32_t spanIdx;
  };

  struct DeviceMemoryBlock {
    VkcAllocation mem;
    std::vector<OffsetSize> layout;
    bool pageReserved;
  };

  struct MemoryPool {
    std::vector<DeviceMemoryBlock> blocks;
  };

  struct AllocatorState {
    VkcCommon *context;

    std::vector<size_t> memTypeAllocSizes;
    uint32_t totalAllocs;

    uint32_t pageSize;
    VkDeviceSize memoryBlockMinSize;

    std::vector<MemoryPool> memPools;
  };

  extern AllocatorState state;

  //ALLOCATOR INTERFACE / INSTALLATION
  void activate(VkcCommon *context);
  void alloc(VkcAllocation &outAlloc, VkcAllocationCreateInfo createInfo);
  void free(VkcAllocation &handle);
  size_t allocatedSize(uint32_t memoryType);
  uint32_t numAllocs();

}
