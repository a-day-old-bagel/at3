
# Don't build an Entity Component System here, only set up the CMake variable to let the game code do so later,
# but do build realtimeutils
set (EZECS_SET_PATH_ONLY ON CACHE BOOL "Only set the variable 'EZECS_SOURCE_DIR' - don't build." FORCE)
add_subdirectory( ezecs )
# Reset that option for the next time ezecs is used (outside of the common targets, actually building the ECS)
set (EZECS_SET_PATH_ONLY OFF CACHE BOOL "Only set the variable 'EZECS_SOURCE_DIR' - don't build." FORCE)
# Add the newly-built realtimeutils to the external targets for convenience
target_link_libraries( ${AT3_TARGET_PREFIX}external INTERFACE realtimeutils )

add_subdirectory( global )
add_subdirectory( gpuapi )
add_subdirectory( vulkan )

add_library( ${AT3_TARGET_PREFIX}common STATIC
  eventResponseMap.cpp eventResponseMap.hpp
  camera.hpp
  debug.hpp
  game.hpp
  meshObject.hpp
  meshObjectVk.hpp
  perspectiveCamera.hpp
  scene.hpp
  sceneObject.hpp
  sdlContext.hpp sdlContext.cpp
  skyBox.hpp
  terrainObject.hpp
  thirdPersonCamera.hpp
  transform.cpp transform.hpp
  transformRAII.cpp transformRAII.hpp
  transformStack.cpp transformStack.hpp
  textureView.cpp textureView.hpp
  imageProcessing.hpp imageProcessing.cpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}common
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}graphics
  )
target_include_directories( ${AT3_TARGET_PREFIX}common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
