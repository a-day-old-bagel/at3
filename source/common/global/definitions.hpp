#pragma once

#include "macros.hpp"

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
#define TEXTURE_ARRAY_LENGTH 1

#if USE_VULKAN_COORDS
# if COMBINE_MESHES
#   define MESH_FLAGS ( aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_Triangulate \
                                                        | aiProcess_FlipUVs )
# else
#   define MESH_FLAGS ( aiProcess_Triangulate | aiProcess_FlipUVs )
# endif
#else
# if COMBINE_MESHES
#   define MESH_FLAGS ( aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_FlipWindingOrder \
                        | aiProcess_Triangulate )
# else
#   define MESH_FLAGS ( aiProcess_FlipWindingOrder | aiProcess_Triangulate )
# endif
#endif

// micro$haft® winderp™  ---  for explanation, see https://tinyurl.com/qf9mkvu
//#undef near
//#undef far
// and more windows stupidity
//#undef min
//#undef max
// If you were going to suggest using their macros that disable these, they don't work, and near and far don't have one.

// Physics and controls defines
#define HUMAN_HEIGHT 1.83f
#define HUMAN_WIDTH 0.5f
#define HUMAN_DEPTH 0.3f

#define CHARA_JUMP 8.f
#define CHARA_MIDAIR_FACTOR 0.05f // 0.02f
#define CHARA_SPRING_FACTOR 140.f
#define CHARA_USE_FOUR_RAYS 0
#define CHARA_WALK 100.f
#define CHARA_RUN 250.f

#define TRACK_VACANT_BRAKE 0.5f
#define TRACK_TORQUE 100.f
#define WHEEL_RADIUS 1.5f

#define PYR_SIDE_ACCEL 2500.f
#define PYR_UP_ACCEL 4000.f

#define FREE_SPEED 5.f
