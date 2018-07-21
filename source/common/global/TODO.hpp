// TODO: precompiled headers
// TODO: runtime-compiled c++
// TODO: ninja build system instead of make
// TODO: vulkan terrain
// TODO: vulkan materials/textures/pbr-only
// TODO: kerning-based terrain generation on vulkan compute
// TODO: voxel stuff: OpenVDB (voxel engine): http://www.openvdb.org/ Subreddit: https://www.reddit.com/r/VoxelGameDev/
// TODO: use steam audio c api for awsome 3d sound stuff
// TODO: Investigate pooled main memory management: TLSF : https://github.com/mattconte/tlsf
// TODO: Investigate parallel CPU task scheduler: enkiTS : https://github.com/dougbinks/enkiTS
// TODO: Investigate a better hash map than unordered_map : sparsepp : https://github.com/greg7mdp/sparsepp
// TODO: Possibly use precompiled sdl and vulkan sdk's in extern instead of relying on system-installed sdk's
// TODO: Stop using custom SDLvulkan.h, now that SDL supports Vulkan - but must find way to force XCB over X11 - search SDL2 repo for "VK_USE_PLATFORM_XCB_KHR"
// TODO: Third-person camera smoothing (per-frame amount that cam moves towards target is proportional to distance from target, maybe have dead zone for anti-jiggle)
// TODO: Use swapchain cacheing
// TODO: Use SLikeNet for multiplayer, or GameNetworkingSockets from Valve
// TODO: Consider a BOB-inspired (horror?) game
// TODO: USE GLTF and PBR
// TODO: investigate the need for custom allocators
// TODO: find a drop-in replacement for std::vector
// TODO: use GPUopen's vulkan memory manager https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
// TODO: use volk for loading function pointers for vulkan: https://github.com/zeux/volk
// TODO: switch to using indirect draw to avoid per-instance draw calls, and possibly only need one call for all meshes.
// TODO: move projection matrix into a global shared uniform thing to avoid duplicating data? (vulkan stuff)
// TODO: make a debug line drawing interface for Vulkan stuff, and link it to bullet
// TODO: replace raw pointers with smart pointers in places like VulkanContext and Obj (re-evaluate if this is a good idea) ???
