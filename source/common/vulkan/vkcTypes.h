#pragma once

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include <string>
#include <unordered_map>

#include "configuration.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "macros.h"

namespace at3 {

  struct VkcCommon;

  enum VkcCmdPoolType {
      Graphics,
      Transfer,
      Present
  };

  struct VkcAllocation {
      VkDeviceMemory handle;
      uint32_t type;
      uint32_t id;
      VkDeviceSize size;
      VkDeviceSize offset;
      VkcCommon *context;
  };

  struct VkcAllocationCreateInfo {
      VkMemoryPropertyFlags usage;
      uint32_t memoryTypeIndex;
      VkDeviceSize size;
  };

  struct VkcAllocatorInterface {
      void (*activate)(VkcCommon *);
      void (*alloc)(VkcAllocation &, VkcAllocationCreateInfo);
      void (*free)(VkcAllocation &);
      size_t (*allocatedSize)(uint32_t);
      uint32_t (*numAllocs)();
  };

  struct VkcDeviceQueues {
      VkQueue graphicsQueue;
      VkQueue transferQueue;
      VkQueue presentQueue;
  };

  struct VkcSurface {
      VkSurfaceKHR surface;
      VkFormat format;
  };

  struct VkcRenderBuffer {
      VkImage handle;
      VkImageView view;
      VkcAllocation imageMemory;
  };

  struct VkcCommandBuffer {
      VkCommandBuffer buffer;
      VkcCmdPoolType owningPool;
      VkcCommon *context;
  };

  struct VkcSwapChainSupportInfo {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
  };

  struct VkcPhysicalDevice {
      VkPhysicalDevice device;
      VkPhysicalDeviceProperties deviceProps;
      VkPhysicalDeviceMemoryProperties memProps;
      VkPhysicalDeviceFeatures features;
      VkcSwapChainSupportInfo swapChainSupport;
      uint32_t queueFamilyCount;
      uint32_t presentQueueFamilyIdx;
      uint32_t graphicsQueueFamilyIdx;
      uint32_t transferQueueFamilyIdx;
  };

  struct VkcSwapChain {
      VkSwapchainKHR swapChain;
      VkFormat imageFormat;
      VkExtent2D extent;
      std::vector<VkImage> imageHandles;
      std::vector<VkImageView> imageViews;
  };

  struct VkcRenderingData {
      std::vector<VkFramebuffer> frameBuffers;
      std::vector<VkCommandBuffer> commandBuffers;
      std::vector<bool> firstFrame;
      at3::VkcRenderBuffer depthBuffer;
      VkRenderPass mainRenderPass;

      VkBuffer ubo;
      at3::VkcAllocation uboAlloc;
  };

  struct VkcMaterialData {
      std::vector<VkDescriptorSet> descSets;
      VkDescriptorSetLayout descSetLayout;
      VkPipelineLayout pipelineLayout;
      VkPipeline graphicsPipeline;
  };

  struct VkcCommon {
      SDL_Window *window;
      int windowWidth;
      int windowHeight;

      VkInstance instance;
      VkcSurface surface;
      VkcPhysicalDevice gpu;
      VkDevice device;
      VkcDeviceQueues deviceQueues;
      VkcSwapChain swapChain;
      VkCommandPool gfxCommandPool;
      VkCommandPool transferCommandPool;
      VkCommandPool presentCommandPool;
      VkQueryPool queryPool;
      VkDescriptorPool descriptorPool;
      VkSemaphore imageAvailableSemaphore;
      VkSemaphore renderFinishedSemaphore;
      std::vector<VkFence> frameFences;

      VkcRenderingData renderData;
      VkcMaterialData matData;
      VkDescriptorBufferInfo bufferInfo;
      VkWriteDescriptorSet setWrite;

      std::vector<VkWriteDescriptorSet> setWriters;

      VkcAllocatorInterface allocator;
  };




  // TODO: Get rid of this if new texture code matches
  struct VkcTextureResource {
    VkImage image;
    at3::VkcAllocation deviceMemory;
    VkImageView view;
    VkFormat format;

    uint32_t width;
    uint32_t height;
    uint32_t numChannels;
  };





  enum class EMeshVertexAttribute : uint8_t {
      POSITION,
      UV0,
      UV1,
      NORMAL,
      TANGENT,
      BITANGENT,
      COLOR
  };

  struct VertexRenderData {
    VkVertexInputAttributeDescription *attrDescriptions;
    EMeshVertexAttribute *attributes;
    uint32_t attrCount;
    uint32_t vertexSize;
  };

  template<typename EcsInterface>
  struct MeshInstance {
    typename EcsInterface::EcsId id = 0;
    uint32_t uboIdx;
  };

  template<typename EcsInterface>
  struct MeshResource {
    VkBuffer buffer;
    VkcAllocation bufferMemory;

    uint32_t vOffset;
    uint32_t iOffset;

    uint32_t vCount;
    uint32_t iCount;

    glm::vec3 min;
    glm::vec3 max;

    std::vector<MeshInstance<EcsInterface>> instances;
  };

  template<typename EcsInterface>
  using MeshResources = std::vector<MeshResource<EcsInterface>>;
  template<typename EcsInterface>
  using MeshRepository = std::unordered_map<std::string, MeshResources<EcsInterface>>;

}
