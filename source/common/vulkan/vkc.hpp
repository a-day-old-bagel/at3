
#pragma once

#include <stb/stb_image.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <SDLvulkan.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <gli/gli.hpp>

#include "topics.hpp"
#include "settings.hpp"

#include "vkcTypes.hpp"
#include "vkcAlloc.hpp"
#include "vkcUboPageMgr.hpp"
#include "vkcPipelines.hpp"
#include "vkcTextures.hpp"

#define SUBSCRIBE_TOPIC(e, x) std::make_unique<rtu::topics::Subscription>(e, RTU_MTHD_DLGT(&VulkanContext::x, this));

namespace at3 {

  template<typename EcsInterface>
  struct VulkanContextCreateInfo {
      std::string appName;
      SDL_Window *window = nullptr;
      EcsInterface *ecs = nullptr;
      std::vector<VkDescriptorPoolSize> descriptorTypeCounts;
      static VulkanContextCreateInfo<EcsInterface> defaults();
  };

  template<typename EcsInterface>
  class VulkanContext {

    public:

      explicit VulkanContext(VulkanContextCreateInfo<EcsInterface> info);
      virtual ~VulkanContext();
      void updateWvMat(void *data);
      void step();
      void reInitRendering();
      void registerMeshInstance(
          typename EcsInterface::EcsId id,
          const std::string &meshFileName,
          const std::string &textureFileName = "");

    private:

      EcsInterface *ecs;
      VkcCommon common;
      std::unique_ptr<UboPageMgr> dataStore;
      MeshRepository<EcsInterface> meshResources;
      std::unique_ptr<VkcTextureRepository<EcsInterface>> textures;


      glm::mat4 currentWvMat = glm::mat4(1.f);
      std::unique_ptr<rtu::topics::Subscription> sub_wvUpdate, sub_windowResize;
      VkDebugReportCallbackEXT callback;
//      GlobalShaderDataStore globalData;
      static const uint32_t INVALID_QUEUE_FAMILY_IDX = (uint32_t) -1;

      void createInstance(const char *appName);
      void createPhysicalDevice();
      void createLogicalDevice();
      void createQueryPool(int queryCount);
      void getWindowSize();
      void createSurface();
      void createSwapchainForSurface();
      VkExtent2D chooseSwapExtent();


      void createDescriptorPool(VkDescriptorPool &outPool, VulkanContextCreateInfo<EcsInterface> &info);

      void createImageView(VkImageView &outView, VkFormat imageFormat, VkImageAspectFlags aspectMask, uint32_t mipCount,
                                 const VkImage &imageHdl);
      void createVkSemaphore(VkSemaphore &outSemaphore);
      void createFence(VkFence &outFence);
      void createCommandPool(VkCommandPool &outPool);
      void freeDeviceMemory(VkcAllocation &mem);
      void createRenderPass(VkRenderPass &outPass, std::vector<VkAttachmentDescription> &colorAttachments,
                                  VkAttachmentDescription *depthAttachment);
      void createCommandBuffer(VkCommandBuffer &outBuffer, VkCommandPool &pool);
      uint32_t getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
                             VkMemoryPropertyFlags requiredProperties);
      void createFrameBuffers(std::vector<VkFramebuffer> &outBuffers, const VkcSwapChain &swapChain,
                                    const VkImageView *depthBufferView, const VkRenderPass &renderPass);
      void allocateDeviceMemory(VkcAllocation &outMem, VkcAllocationCreateInfo info);
      VkcCommandBuffer beginScratchCommandBuffer(VkcCmdPoolType type);
      void submitScratchCommandBuffer(VkcCommandBuffer &commandBuffer);

      //todo: do we really need three of these?
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                      uint32_t dstOffset, VkcCommandBuffer &buffer);
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                      uint32_t dstOffset, VkCommandBuffer &buffer);
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                      uint32_t dstOffset, VkCommandBuffer *buffer);

      void createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties);
      void copyDataToBuffer(VkBuffer *buffer, uint32_t dataSize, uint32_t dstOffset, char *data);
      void createImage(VkImage &outImage, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                       VkImageUsageFlags usage);
      void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
      void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
      void allocMemoryForImage(VkcAllocation &outMem, const VkImage &image, VkMemoryPropertyFlags properties);


      void initRendering(uint32_t num);
      void updateDescriptorSets(UboPageMgr *dataStore);
      void createMainRenderPass();
      void createDepthBuffer();
      void render(UboPageMgr *dataStore, const glm::mat4 &wvMat, const MeshRepository<EcsInterface> &meshAssets,
                  EcsInterface *ecs);


      void setGlobalVertexLayout(std::vector<EMeshVertexAttribute> layout);
      uint32_t make(MeshResource<EcsInterface> &outAsset, float *vertices, uint32_t vertexCount, uint32_t *indices,
                    uint32_t indexCount);
      MeshResources <EcsInterface> loadMesh(const char *filepath, bool combineSubMeshes);
      void quad(MeshResource<EcsInterface> &outAsset, float width, float height, float xOffset, float yOffset);


      void cleanupRendering();

      void createDebugCallback();

      static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
          VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t sourceObject, size_t location,
          int32_t messageCode, const char *layerPrefix, const char *message, void *userData);

  };

  /**
   * Template implementations are included as separate files for the sake of file brevity, and are separated into loose
   * categories as follows:
   */
  // Public member functions of VulkanContext
# include "vkcImplApi.hpp"
  // Member functions of VulkanContext that are generally only called during initialization
# include "vkcImplInternalCallOnce.hpp"
  // Member functions of VulkanContext that are called more than just during initialization
# include "vkcImplInternalDynamic.hpp"
  // Member functions of VulkanContext that deal with asset management
# include "vkcImplInternalResources.hpp"

}

#include "vkcTextures.hpp"
