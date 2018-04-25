
#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <cstdint>
#include <deque>
#include "vkcMaterial.h"

//#include "configuration.h"
////#include "shader_inputs.h"
//#include "vkcTypes.h"


namespace at3 {

  uint32_t getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
                         VkMemoryPropertyFlags requiredProperties);

  void createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkcCommon &ctxt);

  class DataStore {

      uint32_t slotsPerPage;
      uint32_t size;

      VkcCommon *ctxt;

      struct UBOPage {
        VkBuffer buf;
        VkcAllocation alloc;
        std::deque<uint32_t> freeIndices;
        uint32_t index;

#       if DEVICE_LOCAL
#         if PERSISTENT_STAGING_BUFFER
        void* map;
#         else
        char* map;
#         endif
#       else
        void *map;
#       endif

#       if PERSISTENT_STAGING_BUFFER
        VkBuffer stagingBuf;
        Allocation stagingAlloc;
#       endif
      };

      std::vector<UBOPage> pages;

      UBOPage &createNewPage() {
        UBOPage page;
        VkcCommon &_ctxt = *ctxt;

        createBuffer(
            page.buf,
            page.alloc,
            size,
#           if DEVICE_LOCAL
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
#           else
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#           endif
            _ctxt);

#       if PERSISTENT_STAGING_BUFFER
        createBuffer(
          page.stagingBuf,
          page.stagingAlloc,
          size,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
#           if COPY_ON_MAIN_COMMANDBUFFER
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#           else
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
#           endif
          _ctxt );
#       endif

#       if DEVICE_LOCAL
#         if PERSISTENT_STAGING_BUFFER
        vkMapMemory(_ctxt.device, page.stagingAlloc.handle, page.stagingAlloc.offset, page.stagingAlloc.size, 0, &page.map);
#         else
        page.map = (char*)malloc(size);
#         endif
#       else
        vkMapMemory(_ctxt.device, page.alloc.handle, page.alloc.offset, page.alloc.size, 0, &page.map);
#       endif

        for (uint32_t i = 0; i < slotsPerPage; ++i) {
          page.freeIndices.push_back(i);
        }

        page.index = (uint32_t) pages.size();

        pages.push_back(page);
        return pages[page.index];
      }

    public:

      enum AcquireStatus {
        SUCCESS, NEWPAGE, FAILURE
      };

      DataStore(VkcCommon &_ctxt) {
        ctxt = &_ctxt;
        slotsPerPage = 256;
        size = (sizeof(VShaderInput) * slotsPerPage);
      }


      VkcAllocation &getAlloc(uint32_t idx) {
        AT3_ASSERT(pages.size() >= (idx + 1), "Array index out of bounds");
        return pages[idx].alloc;
      }

      VkBuffer &getPage(uint32_t idx) {
        AT3_ASSERT(pages.size() >= (idx + 1), "Array index out of bounds");
        return pages[idx].buf;
      }


      AcquireStatus acquire(uint32_t &outIdx) {
        UBOPage *p = nullptr;

        for (auto &page : pages) {
          if (page.freeIndices.size() > 0) {
            p = &page;
            break;
          }
        }

        bool newPage = false;
        if (!p) {
          p = &createNewPage();
          newPage = true;
        }

        uint32_t slot = p->freeIndices.front();
        p->freeIndices.pop_front();

        outIdx = slot << 16;
        outIdx += p->index;

        return newPage ? NEWPAGE : SUCCESS;
      }

      uint32_t getNumPages() {
        return (uint32_t) pages.size();
      }

      template<typename EcsInterface>
      void updateBuffers(const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix, VkCommandBuffer *commandBuffer,
                         VkcCommon &ctxt, EcsInterface *ecs, const MeshRepository<EcsInterface> &meshRepo) {
        std::vector<VkMappedMemoryRange> rangesToUpdate;
        rangesToUpdate.resize(pages.size());

        std::vector<VShaderInput *> objPtrs;
        for (uint32_t p = 0; p < pages.size(); ++p) {
          UBOPage page = pages[p];
          objPtrs.push_back((VShaderInput *) page.map);

          VkMappedMemoryRange curRange = {};
          curRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
          curRange.memory = page.alloc.handle;
          curRange.offset = page.alloc.offset;
          curRange.size = page.alloc.size;
          curRange.pNext = nullptr;

          rangesToUpdate[p] = curRange;
        }

        for (auto pair : meshRepo) {
          for (auto mesh : pair.second) {
            for (auto instance : mesh.instances) {
              glm::uint32 uboSlot = instance.uboIdx >> 16;
              glm::uint32 uboPage = instance.uboIdx & 0xFFFF;
              glm::mat4 modelViewMatrix = viewMatrix * ecs->getAbsTransform(instance.id);
              objPtrs[uboPage][uboSlot].model = projMatrix * modelViewMatrix;
              objPtrs[uboPage][uboSlot].normal = glm::transpose(glm::inverse(modelViewMatrix));
            }
          }
        }

#       if !DEVICE_LOCAL || PERSISTENT_STAGING_BUFFER
        vkFlushMappedMemoryRanges(ctxt.device, rangesToUpdate.size(), rangesToUpdate.data());
#       endif

#       if DEVICE_LOCAL
        for (uint32_t p = 0; p < pages.size(); ++p) {
#           if PERSISTENT_STAGING_BUFFER
            copyBuffer(pages[p].stagingBuf, pages[p].buf, size, 0, 0, commandBuffer, ctxt);
#           else
            copyDataToBuffer(&pages[p].buf, size, 0, (char*)pages[p].map, ctxt);
#           endif
        }
#       endif
      }

      VkDescriptorType getDescriptorType() {
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      }
  };
}
