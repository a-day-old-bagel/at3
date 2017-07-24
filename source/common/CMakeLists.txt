set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${epoxy_DEFINITIONS}" )

add_subdirectory( global )
add_subdirectory(gpuapi)

add_library( ${AT3_TARGET_PREFIX}common STATIC
  activeControl.cpp activeControl.h
  camera.h
  debug.h
  game.h
  meshObject.h
  perspectiveCamera.h
  scene.h
  sceneObject.h
  skyBox.h
  terrainObject.h
  thirdPersonCamera.h
  transform.cpp transform.h
  transformRAII.cpp transformRAII.h
  transformStack.cpp transformStack.h
  )
target_link_libraries( ${AT3_TARGET_PREFIX}common
  ${AT3_TARGET_PREFIX}graphics
  assimp
  fastnoise
  realtimeutils
  )
target_include_directories( ${AT3_TARGET_PREFIX}common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
set_property( TARGET ${AT3_TARGET_PREFIX}common PROPERTY CXX_STANDARD 17 )
set_property( TARGET ${AT3_TARGET_PREFIX}common PROPERTY CXX_STANDARD_REQUIRED ON )
