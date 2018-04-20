#pragma once

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vector>

namespace at3 {

  struct VkhContext;

  enum ECommandPoolType {
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
      VkhContext *context;
  };

  struct AllocationCreateInfo {
      VkMemoryPropertyFlags usage;
      uint32_t memoryTypeIndex;
      VkDeviceSize size;
  };

  struct AllocatorInterface {
      void (*activate)(VkhContext *);

      void (*alloc)(Allocation &, AllocationCreateInfo);

      void (*free)(Allocation &);

      size_t (*allocatedSize)(uint32_t);

      uint32_t (*numAllocs)();
  };

  struct VkhDeviceQueues {
      VkQueue graphicsQueue;
      VkQueue transferQueue;
      VkQueue presentQueue;
  };

  struct VkhSurface {
      VkSurfaceKHR surface;
      VkFormat format;
  };

  struct VkhRenderBuffer {
      VkImage handle;
      VkImageView view;
      Allocation imageMemory;
  };

  struct VkhCommandBuffer {
      VkCommandBuffer buffer;
      ECommandPoolType owningPool;
      VkhContext *context;
  };

  struct VkhSwapChainSupportInfo {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
  };

  struct VkhPhysicalDevice {
      VkPhysicalDevice device;
      VkPhysicalDeviceProperties deviceProps;
      VkPhysicalDeviceMemoryProperties memProps;
      VkPhysicalDeviceFeatures features;
      VkhSwapChainSupportInfo swapChainSupport;
      uint32_t queueFamilyCount;
      uint32_t presentQueueFamilyIdx;
      uint32_t graphicsQueueFamilyIdx;
      uint32_t transferQueueFamilyIdx;
  };

  struct VkhSwapChain {
      VkSwapchainKHR swapChain;
      VkFormat imageFormat;
      VkExtent2D extent;
      std::vector<VkImage> imageHandles;
      std::vector<VkImageView> imageViews;
  };

  struct VkhRenderingData {
      std::vector<VkFramebuffer> frameBuffers;
      std::vector<VkCommandBuffer> commandBuffers;
      at3::VkhRenderBuffer depthBuffer;
      VkRenderPass mainRenderPass;

      VkBuffer ubo;
      at3::Allocation uboAlloc;
  };

  struct VkhMatData {
      std::vector<VkDescriptorSet> descSets;
      VkDescriptorSetLayout descSetLayout;
      VkPipelineLayout pipelineLayout;
      VkPipeline graphicsPipeline;
  };

  struct VkhContext {
      SDL_Window *window;
      int windowWidth;
      int windowHeight;

      VkInstance instance;
      VkhSurface surface;
      VkhPhysicalDevice gpu;
      VkDevice device;
      VkhDeviceQueues deviceQueues;
      VkhSwapChain swapChain;
      VkCommandPool gfxCommandPool;
      VkCommandPool transferCommandPool;
      VkCommandPool presentCommandPool;
      VkQueryPool queryPool;
      VkDescriptorPool descriptorPool;
      VkSemaphore imageAvailableSemaphore;
      VkSemaphore renderFinishedSemaphore;
      std::vector<VkFence> frameFences;

      VkhRenderingData renderData;
      VkhMatData matData;
      VkDescriptorBufferInfo bufferInfo;
      VkWriteDescriptorSet setWrite;

      AllocatorInterface allocator;
  };
}
