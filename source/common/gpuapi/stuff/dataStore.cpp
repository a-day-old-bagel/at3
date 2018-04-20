
#include "dataStore.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <deque>
#include "shader_inputs.h"
#include "config.h"

namespace ubo_store {
  uint32_t uboArrayLen;
  uint32_t countPerPage;
  uint32_t numPages;
  uint32_t size;

  //indexes into a dynamic ubo must be a multiple of minUniformBufferOffsetAlignment
  //so each slot may need to be slightly larger than the size of VShaderInput
  uint32_t slotSize;

  at3::VkhContext *ctxt;

  struct UBOPage {
    VkBuffer buf;
    at3::Allocation alloc;
    std::deque<uint32_t> freeIndices;

#if DEVICE_LOCAL
#if PERSISTENT_STAGING_BUFFER
    void* map;

#else
    char* map;
#endif
#else
    void *map;
#endif

#if PERSISTENT_STAGING_BUFFER
    VkBuffer stagingBuf;
    at3::Allocation stagingAlloc;
#endif
  };

  std::vector<UBOPage> pages;

  void init(at3::VkhContext &_ctxt) {
    ctxt = &_ctxt;

    size_t uboAlignment = _ctxt.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;
    size_t dynamicAlignment = ((sizeof(VShaderInput) / uboAlignment) * uboAlignment) +
                              (((sizeof(VShaderInput) % uboAlignment) > 0 ? uboAlignment : 0));

    slotSize = dynamicAlignment;

    countPerPage = 256;
    size = (sizeof(VShaderInput) * countPerPage);
  }

  UBOPage &createNewPage() {
    UBOPage page;
    at3::VkhContext &_ctxt = *ctxt;

    at3::createBuffer(
        page.buf,
        page.alloc,
        size,
#if DEVICE_LOCAL
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
#else
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#endif
        _ctxt);

#if PERSISTENT_STAGING_BUFFER
    at3::createBuffer(
      page.stagingBuf,
      page.stagingAlloc,
      size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
#if COPY_ON_MAIN_COMMANDBUFFER
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#else
      VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#endif
      _ctxt
    );
#endif

#if DEVICE_LOCAL
#if PERSISTENT_STAGING_BUFFER
    vkMapMemory(_ctxt.device, page.stagingAlloc.handle, page.stagingAlloc.offset, page.stagingAlloc.size, 0, &page.map);
#else
    page.map = (char*)malloc(size);
#endif
#else
    vkMapMemory(_ctxt.device, page.alloc.handle, page.alloc.offset, page.alloc.size, 0, &page.map);
#endif

    for (uint32_t i = 0; i < countPerPage; ++i) {
      page.freeIndices.push_back(i);
    }

    pages.push_back(page);
    return pages[pages.size() - 1];
  }

  at3::Allocation &getAlloc(uint32_t idx) {
    checkf(pages.size() >= (idx + 1), "Array index out of bounds");
    return pages[idx].alloc;
  }

  VkBuffer &getPage(uint32_t idx) {
    checkf(pages.size() >= (idx + 1), "Array index out of bounds");
    return pages[idx].buf;
  }

  bool acquire(uint32_t &outIdx) {
    UBOPage *p = nullptr;

    for (auto &page : pages) {
      if (page.freeIndices.size() > 0) {
        p = &page;
        break;
      }
    }

    if (!p) {
      p = &createNewPage();
    }

    uint32_t slot = p->freeIndices.front();
    p->freeIndices.pop_front();

    outIdx = slot << 3;
    outIdx += (slot / countPerPage);

    return true;

  }

  uint32_t getNumPages() {
    return pages.size();
  }

  void updateBuffers(const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix, VkCommandBuffer *commandBuffer,
                     at3::VkhContext &ctxt) {
    std::vector<VkMappedMemoryRange> rangesToUpdate;
    rangesToUpdate.resize(pages.size());

    for (uint32_t p = 0; p < pages.size(); ++p) {
      UBOPage page = pages[p];
      VShaderInput *objPtr = (VShaderInput *) page.map;

      for (uint32_t i = 0; i < countPerPage; ++i) {
        objPtr[i].model = projMatrix * viewMatrix;
        objPtr[i].normal = glm::transpose(glm::inverse(viewMatrix));
      }

      VkMappedMemoryRange curRange = {};
      curRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      curRange.memory = page.alloc.handle;
      curRange.offset = page.alloc.offset;
      curRange.size = page.alloc.size;
      curRange.pNext = nullptr;

      rangesToUpdate[p] = curRange;
    }

#if !DEVICE_LOCAL || PERSISTENT_STAGING_BUFFER
    vkFlushMappedMemoryRanges(ctxt.device, rangesToUpdate.size(), rangesToUpdate.data());
#endif

#if DEVICE_LOCAL
    for (uint32_t p = 0; p < pages.size(); ++p)
      {
#if PERSISTENT_STAGING_BUFFER
          at3::copyBuffer(pages[p].stagingBuf, pages[p].buf, size, 0, 0, commandBuffer, ctxt);
#else
          at3::copyDataToBuffer(&pages[p].buf, size, 0, (char*)pages[p].map, ctxt);
#endif
      }
#endif
  }

  VkDescriptorType getDescriptorType() {
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  }
}
