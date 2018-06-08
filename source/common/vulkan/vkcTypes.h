#pragma once

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>
#include <string>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utilities.h"
#include "VulkanTexture.hpp"

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




//  // TODO: Get rid of this if new texture code matches
//  struct VkcTextureResource {
//    VkImage image;
//    at3::VkcAllocation deviceMemory;
//    VkImageView view;
//    VkFormat format;
//
//    uint32_t width;
//    uint32_t height;
//    uint32_t numChannels;
//  };





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

  /**
   *    32 bits are divided between a 12-bit texture index (T), an 11-bit page index (P), and a 9-bit slot index (S).
   *    |T|T|T|T|T|T|T|T|T|T|T|T|P|P|P|P|P|P|P|P|P|P|P|S|S|S|S|S|S|S|S|S|   --> least significant
   *
   *    This provides support for 4096 unique textures and 1048576 mesh instances.
   *    If more are required, this should not be too difficult to change. You should really only have to change things
   *    here and in the shaders. Just leave uint32_t as the getter return types and method argument types.
   */
  struct MeshInstanceIndices {
    typedef uint32_t rawType;
    rawType raw;
    MeshInstanceIndices() : raw(std::numeric_limits<rawType>::max()) {
      // set to max so that things will probably crash if an index is used without first setting it to a valid value.
    }
    inline void set(const uint32_t page, const uint32_t slot, const uint32_t texture = 0) {
      raw = texture << 20u;
      raw += page << 9u;
      raw += slot;
    }
    inline void setTexture(const uint32_t texture) {
      rawType uboPart = raw & 0xFFFFFu;
      raw = texture << 20u | uboPart;
    }
    inline uint32_t getTexture() const {
      return raw >> 20u & 0xFFFu;
    }
    inline uint32_t getPage() const {
      return raw >> 9u & 0x7FFu;
    }
    inline uint32_t getSlot() const {
      return raw & 0x1FFu;
    }
  };

  template<typename EcsInterface>
  struct MeshInstance {
    typename EcsInterface::EcsId id = 0;
    MeshInstanceIndices indices;
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
