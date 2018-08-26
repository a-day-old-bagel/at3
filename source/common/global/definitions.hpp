#pragma once

#include "macros.hpp"

// Platform stuff
#ifdef WIN32
# define AT3_NET_THREAD_PRIORITY THREAD_PRIORITY_NORMAL
#else
# include <sched.h>
# define AT3_NET_THREAD_PRIORITY SCHED_OTHER
#endif

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

// TODO: check if these are even doing anything - are micro$haft® headers being included before this file somehow?
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
