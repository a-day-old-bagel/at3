
#pragma once

#include <cstdint>
#include <deque>
#include "vkcPipelines.hpp"
#include "math.hpp"

namespace at3::vkc {

  uint32_t getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
                         VkMemoryPropertyFlags requiredProperties);

  void createBuffer(VkBuffer &outBuffer, Allocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, Common &ctxt);

  class UboPageMgr {

      uint32_t slotsPerPage;
      uint32_t size;

      Common *ctxt;

      struct Page {
        VkBuffer buf;
        Allocation alloc;
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

      std::vector<Page> pages;

      Page &createNewPage() {
        Page page;
        Common &_ctxt = *ctxt;

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

      UboPageMgr(Common &_ctxt) {
        ctxt = &_ctxt;
        slotsPerPage = 512;
        size = (sizeof(VShaderInput) * slotsPerPage);
      }


      Allocation &getAlloc(uint32_t idx) {
        AT3_ASSERT(pages.size() >= (idx + 1), "Array index out of bounds");
        return pages[idx].alloc;
      }

      VkBuffer &getPage(uint32_t idx) {
        AT3_ASSERT(pages.size() >= (idx + 1), "Array index out of bounds");
        return pages[idx].buf;
      }


      AcquireStatus acquire(MeshInstanceIndices &outIdx) {
        Page *p = nullptr;

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

        outIdx.set(p->index, slot);

        return newPage ? NEWPAGE : SUCCESS;
      }

      uint32_t getNumPages() {
        return (uint32_t) pages.size();
      }

      template<typename EcsInterface>
      void updateBuffers(const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix, VkCommandBuffer *commandBuffer,
                         Common &ctxt, EcsInterface *ecs, const MeshRepository<EcsInterface> &meshRepo) {
        std::vector<VkMappedMemoryRange> rangesToUpdate;
        rangesToUpdate.resize(pages.size());

        std::vector<VShaderInput *> objPtrs;
        for (uint32_t p = 0; p < pages.size(); ++p) {
          Page page = pages[p];
          objPtrs.push_back((VShaderInput *) page.map);

          VkMappedMemoryRange curRange = {};
          curRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
          curRange.memory = page.alloc.handle;
          curRange.offset = page.alloc.offset;
          curRange.size = page.alloc.size;
          curRange.pNext = nullptr;

          rangesToUpdate[p] = curRange;
        }

        // TODO: update only those which have moved, or at least don't calculate for those that haven't. OPTIMIZE!
        for (auto pair : meshRepo) {
          for (auto mesh : pair.second) {
            for (auto instance : mesh.instances) {
              glm::uint32 uboSlot = instance.indices.getSlot();
              glm::uint32 uboPage = instance.indices.getPage();
              objPtrs[uboPage][uboSlot].vp = projMatrix * viewMatrix;
//              objPtrs[uboPage][uboSlot].custom = glm::transpose(glm::inverse(modelViewMatrix));
              objPtrs[uboPage][uboSlot].m = ecs->getAbsTransform(instance.id);
            }
          }
        }

#       if !DEVICE_LOCAL || PERSISTENT_STAGING_BUFFER
        vkFlushMappedMemoryRanges(ctxt.device, (uint32_t)rangesToUpdate.size(), rangesToUpdate.data());
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
