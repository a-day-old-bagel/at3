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

#include "utilities.hpp"
#include "vkcTextures.hpp"

namespace at3::vkc {

  struct Common;

  enum CmdPoolType {
      Graphics,
      Transfer,
      Present
  };

  struct Allocation {
      VkDeviceMemory handle;
      uint32_t type;
      uint32_t id;
      VkDeviceSize size;
      VkDeviceSize offset;
      Common *context;
  };

  struct AllocationCreateInfo {
      VkMemoryPropertyFlags usage;
      uint32_t memoryTypeIndex;
      VkDeviceSize size;
  };

  struct AllocatorInterface {
      void (*activate)(Common *);
      void (*alloc)(Allocation &, AllocationCreateInfo);
      void (*free)(Allocation &);
      size_t (*allocatedSize)(uint32_t);
      uint32_t (*numAllocs)();
  };

  struct DeviceQueues {
      VkQueue graphicsQueue;
      VkQueue transferQueue;
      VkQueue presentQueue;
  };

  struct Surface {
      VkSurfaceKHR handle;
      VkFormat format;
  };

  struct RenderBuffer {
      VkImage handle;
      VkImageView view;
      Allocation imageMemory;
  };

  struct CommandBuffer {
      VkCommandBuffer buffer;
      CmdPoolType owningPool;
      Common *context;
  };

  struct SwapChainSupportInfo {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
  };

  struct PhysicalDevice {
      VkPhysicalDevice device;
      VkPhysicalDeviceProperties deviceProps;
      VkPhysicalDeviceMemoryProperties memProps;
      VkPhysicalDeviceFeatures features;
      SwapChainSupportInfo swapChainSupport;
      uint32_t queueFamilyCount;
      uint32_t presentQueueFamilyIdx;
      uint32_t graphicsQueueFamilyIdx;
      uint32_t transferQueueFamilyIdx;
  };

  struct SwapChain {
      VkSwapchainKHR swapChain;
      VkFormat imageFormat;
      VkExtent2D extent;
      std::vector<VkImage> imageHandles;
      std::vector<VkImageView> imageViews;
  };

  struct WindowSizeDependents {
      std::vector<VkFramebuffer> frameBuffers;
      std::vector<VkCommandBuffer> commandBuffers;
      std::vector<bool> firstFrame;
      RenderBuffer depthBuffer;
  };

  struct Common {
      SDL_Window *window;
      int windowWidth;
      int windowHeight;

      VkInstance instance;
      Surface surface;
      PhysicalDevice gpu;
      VkDevice device;
      DeviceQueues deviceQueues;
      SwapChain swapChain;
      VkCommandPool gfxCommandPool;
      VkCommandPool transferCommandPool;
      VkCommandPool presentCommandPool;
      VkQueryPool queryPool;
      VkDescriptorPool descriptorPool;
      VkSemaphore imageAvailableSemaphore;
      VkSemaphore renderFinishedSemaphore;
      std::vector<VkFence> frameFences;

      WindowSizeDependents windowDependents;
      std::vector<VkWriteDescriptorSet> setWriters; // Only kept to avoid reallocating every frame (what compiler?)

      AllocatorInterface allocator;
  };








  enum class EMeshVertexAttribute : uint8_t {
      INVALID_ATTRIBUTE,
      POSITION,
      UV0,
      UV1,
      NORMAL,
      TANGENT,
      BITANGENT,
      COLOR
  };

  struct VertexAttributes {
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
    Allocation bufferMemory;

    uint32_t vOffset;
    uint32_t iOffset;

    uint32_t vCount;
    uint32_t iCount;

    glm::vec3 min;
    glm::vec3 max;

    std::vector<MeshInstance<EcsInterface>> instances;
  };

  // TODO: Get this disentangled from VulkanContext like TextureRepository, do it when upgrading to gltf
  template<typename EcsInterface>
  using MeshResources = std::vector<MeshResource<EcsInterface>>;
  template<typename EcsInterface>
  using MeshRepository = std::unordered_map<std::string, MeshResources<EcsInterface>>;

}
