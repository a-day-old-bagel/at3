// TODO: precompiled headers
// TODO: runtime-compiled c++
// TODO: ninja build system instead of make
// TODO: vulkan terrain
// TODO: vulkan materials/textures/pbr-only
// TODO: kernel-based terrain generation on vulkan compute
// TODO: voxel stuff: OpenVDB (voxel engine): http://www.openvdb.org/ Subreddit: https://www.reddit.com/r/VoxelGameDev/
// TODO: use steam audio c api for awsome 3d sound stuff
// TODO: Investigate pooled main memory management: TLSF : https://github.com/mattconte/tlsf
// TODO: Investigate parallel CPU task scheduler: enkiTS : https://github.com/dougbinks/enkiTS
// TODO: Investigate a better hash map than unordered_map : sparsepp : https://github.com/greg7mdp/sparsepp
// TODO: Possibly use precompiled sdl and vulkan sdk's in extern instead of relying on system-installed sdk's
// TODO: Stop using custom SDLvulkan.h, now that SDL supports Vulkan - but must find way to force XCB over X11 - search SDL2 repo for "VK_USE_PLATFORM_XCB_KHR"
// TODO: Third-person camera smoothing (per-frame amount that cam moves towards target is proportional to distance from target, maybe have dead zone for anti-jiggle)
// TODO: Use swapchain cache-ing
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
// TODO: replace raw pointers with smart pointers in places like VulkanContext and SceneObject (re-evaluate if this is a good idea) ???
// TODO: the reversed-linear-z depth near/far plane switching trick (check discord history)
// TODO: use the RakNet LobbyServer to manage logins, lobbies, matchmaking, accounts, etc.
// TODO: write the component hash maps on top of a normal contiguous array/vector for iteration's sake
// TODO: make entityId's grow from both ends of the integer range, one end for networked entities and the other end for purely local entities. This can also be used for client ECS requests, in that the client can make the temporary entity on the local end of the collection and replace it with network-end entity when a response comes back.
// TODO: make ezecs state getters const methods
// TODO: make vehicle wheels a local-only entity that is created on each host when the appropriate controls appear
// TODO: profile and possibly replace the string hashing going on for topics/subscriptions
// TODO: investigate the out-of-order deletions that happen on the client when constructs are deleted on the server. These do not currently cause problems other than that the freed ids are pushed onto the stack in a different order, leading to a different creation order of new entities afterwards, which, again, doesn't cause any apparent trouble. But it might be better if that didn't happen since it could cause hard-to-diagnose problems later
