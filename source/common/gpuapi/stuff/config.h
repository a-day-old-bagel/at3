#pragma once

#define SCREEN_W 1920
#define SCREEN_H 1080

#define USE_CUSTOM_SDL_VULKAN 1
#define USE_AT3_COORDS 1

#define BISTRO_TEST 0

// There's a validation error when you use DEVICE_LOCAL and PERSISTENT_STAGING_BUFFER, at the call
// vkFlushMappedMemoryRanges. If you switch, you debug.
#define DEVICE_LOCAL 0
#define PRINT_VK_FRAMETIMES 1
#define PERSISTENT_STAGING_BUFFER 0
#define COPY_ON_MAIN_COMMANDBUFFER 0
#define COMBINE_MESHES 0
