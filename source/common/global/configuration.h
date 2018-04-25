#pragma once

#include "macros.h"

// Vulkan stuff
#define USE_CUSTOM_SDL_VULKAN 1
#define USE_VULKAN_COORDS 1
#define AT3_DEBUG_WINDOW_EVENTS 0
// There's a validation error when you use DEVICE_LOCAL and PERSISTENT_STAGING_BUFFER, at the call
// vkFlushMappedMemoryRanges. If you switch, you debug.
#define DEVICE_LOCAL 0
#define PRINT_VK_FRAMETIMES 0
#define PERSISTENT_STAGING_BUFFER 0
#define COPY_ON_MAIN_COMMANDBUFFER 0
#define COMBINE_MESHES 0

#if USE_VULKAN_COORDS
# if COMBINE_MESHES
#   define MESH_FLAGS aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_Triangulate
# else
#   define MESH_FLAGS aiProcess_Triangulate
# endif
#else
# if COMBINE_MESHES
#   define MESH_FLAGS aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_FlipWindingOrder \
                      | aiProcess_Triangulate
# else
#   define MESH_FLAGS aiProcess_FlipWindingOrder | aiProcess_Triangulate
# endif
#endif

// micro$haft® winderp™  ---  http://lolengine.net/blog/2011/3/4/fuck-you-microsoft-near-far-macros
#undef near
#undef far

// Physics and controls defines
#define HUMAN_HEIGHT 1.83f
#define HUMAN_WIDTH 0.5f
#define HUMAN_DEPTH 0.3f

#define CHARA_JUMP 8.f
#define CHARA_MIDAIR_FACTOR 0.f // 0.02f

#define TRACK_VACANT_BRAKE 0.5f

#define PYR_SIDE_ACCEL 2500.f
#define PYR_UP_ACCEL 4000.f
#define TRACK_TORQUE 100.f
#define CHARA_WALK 100.f
#define CHARA_RUN 250.f
#define FREE_SPEED 5.f

#define WHEEL_RADIUS 1.5f
