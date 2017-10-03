#pragma once

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

void vulkan_main(SDL_Window *window, PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback,
                 PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback,
                 VkDebugReportCallbackEXT msg_callback,
                 PFN_vkDebugReportMessageEXT DebugReportMessage);

#ifdef __cplusplus
}
#endif