
#pragma once

#include <stb/stb_image.h>
#include <unordered_map>
#include <string>
#include <SDLvulkan.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "configuration.h"
#include "macros.h"
#include "topics.hpp"
#include "utilities.h"

#include "vkcTypes.h"
#include "vkcAlloc.h"
#include "vkcDataStore.h"
#include "vkcInitializers.h"

//#include "vkh_mesh.h"
//#include "vkh_material.h"
//#include "mesh_loading.h"

//#include "vkcShaderSources.h"
#include "vkcMaterial.h"




#define SUBSCRIBE_TOPIC(e, x) std::make_unique<rtu::topics::Subscription>(e, RTU_MTHD_DLGT(&VulkanContext::x, this));

namespace at3 {

  template <typename EcsInterface>
  struct VulkanContextCreateInfo {
    std::string appName;
    SDL_Window *window = nullptr;
    EcsInterface *ecs = nullptr;
    std::vector<VkDescriptorType> types;
    std::vector<uint32_t> typeCounts;

    static VulkanContextCreateInfo<EcsInterface> defaults();
  };

  template <typename EcsInterface>
  class VulkanContext {

    public:

      explicit VulkanContext(VulkanContextCreateInfo<EcsInterface> info);
      virtual ~VulkanContext();
      void updateWvMat(void* data);
      void step();
      void reInitRendering();
      void registerMeshInstance(const std::string &meshFileName, const typename EcsInterface::EcsId id);

    private:


//      typedef std::vector<MeshResource<EcsInterface>> MeshResources;
//      typedef std::unordered_map<std::string, MeshResources> MeshRepository;


      EcsInterface *ecs;
      VkcCommon common;
      std::unique_ptr<DataStore> dataStore;
      MeshRepository<EcsInterface> meshResources;
      glm::mat4 currentWvMat;
      std::unique_ptr<rtu::topics::Subscription> sub_wvUpdate, sub_windowResize;

      static const uint32_t INVALID_QUEUE_FAMILY_IDX = (uint32_t)-1;
      VkDebugReportCallbackEXT callback;

      GlobalShaderDataStore globalData;

//      VertexRenderData *_vkRenderData;





      void createInstance(const char *appName);
      void createPhysicalDevice();
      void createLogicalDevice();
      void createQueryPool(int queryCount);

      void getWindowSize();
      void createSurface();
      void createSwapchainForSurface();
      VkExtent2D chooseSwapExtent();



      void createDescriptorPool(VkDescriptorPool &outPool, const VkDevice &device,
                                std::vector<VkDescriptorType> &descriptorTypes, std::vector<uint32_t> &maxDescriptors);
      void createImageView(VkImageView &outView, VkFormat imageFormat, VkImageAspectFlags aspectMask, uint32_t mipCount,
                           const VkImage &imageHdl, const VkDevice &device);
      void createVkSemaphore(VkSemaphore &outSemaphore, const VkDevice &device);
      void createFence(VkFence &outFence, VkDevice &device);
      void createCommandPool(VkCommandPool &outPool, const VkDevice &lDevice, const VkcPhysicalDevice &physDevice,
                             uint32_t queueFamilyIdx);
      void freeDeviceMemory(VkcAllocation &mem);
      void createRenderPass(VkRenderPass &outPass, std::vector<VkAttachmentDescription> &colorAttachments,
                            VkAttachmentDescription *depthAttachment, const VkDevice &device);
      void createCommandBuffer(VkCommandBuffer &outBuffer, VkCommandPool &pool, const VkDevice &lDevice);
      uint32_t getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
                             VkMemoryPropertyFlags requiredProperties);
      void createFrameBuffers(std::vector<VkFramebuffer> &outBuffers, const VkcSwapChain &swapChain,
                              const VkImageView *depthBufferView, const VkRenderPass &renderPass, const VkDevice &device);
      void allocateDeviceMemory(VkcAllocation &outMem, VkcAllocationCreateInfo info, VkcCommon &ctxt);
      VkcCommandBuffer beginScratchCommandBuffer(VkcCmdPoolType type, VkcCommon &ctxt);
      void submitScratchCommandBuffer(VkcCommandBuffer &commandBuffer);

      //todo: do we really need three of these?
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset,
                      VkcCommandBuffer &buffer);
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset,
                      VkCommandBuffer &buffer);
      void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset, uint32_t dstOffset,
                      VkCommandBuffer *buffer, VkcCommon &ctxt);

      void createShaderModule(VkShaderModule &outModule, unsigned char *binaryData, size_t dataSize, const VkcCommon &ctxt);
      void createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkcCommon &ctxt);
      void copyDataToBuffer(VkBuffer *buffer, uint32_t dataSize, uint32_t dstOffset, char *data, VkcCommon &ctxt);
      void createImage(VkImage &outImage, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                       VkImageUsageFlags usage, const VkcCommon &ctxt);
      void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkcCommon &ctxt);
      void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                 VkcCommon &ctxt);
      void
      allocMemoryForImage(VkcAllocation &outMem, const VkImage &image, VkMemoryPropertyFlags properties, VkcCommon &ctxt);
      void createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, const VkcCommon &ctxt);




      void initRendering(VkcCommon &, uint32_t num);
      void updateDescriptorSets(VkcCommon &ctxt, DataStore *dataStore);
      void createMainRenderPass(VkcCommon &ctxt);
      void createDepthBuffer(VkcCommon &ctxt);
      void render(VkcCommon &ctxt, DataStore* dataStore, const glm::mat4 &wvMat,
                  const MeshRepository<EcsInterface> &meshAssets, EcsInterface *ecs);





      void makeTexture(VkcTextureResource &outAsset, const char *filepath, VkcCommon &ctxt);


      void initGlobalShaderData(VkcCommon& ctxt);
//      void createBasicMaterial(unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen,
//                               VkcCommon &ctxt, VkhMaterialCreateInfo &createInfo);
//      void loadUBOTestMaterial(VkcCommon &ctxt, int num);


      MeshResources<EcsInterface> loadMesh(const char* filepath, bool combineSubMeshes, at3::VkcCommon& ctxt);
      void setGlobalVertexLayout(std::vector<EMeshVertexAttribute> layout);
//      const VertexRenderData *vertexRenderData();
      uint32_t make(MeshResource<EcsInterface> &outAsset, VkcCommon &ctxt, float *vertices, uint32_t vertexCount,
                    uint32_t *indices, uint32_t indexCount);
      void quad(MeshResource<EcsInterface> &outAsset, VkcCommon &ctxt, float width = 2.0f, float height = 2.0f,
                float xOffset = 0.0f, float yOffset = 0.0f);







      void cleanupRendering();

      void createDebugCallback();
      static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
          VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t sourceObject, size_t location,
          int32_t messageCode, const char *layerPrefix, const char *message, void *userData);

  };

  // IMPLEMENTATION TEMPLATES INCLUDED HERE
# include "vkcImplApi.h"
# include "vkcImplInternal.h"

}
