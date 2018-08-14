
add_library( ${AT3_TARGET_PREFIX}scene STATIC
  sceneObject.hpp
  sceneTree.hpp
  transform.cpp transform.hpp
  transformRAII.cpp transformRAII.hpp
  transformStack.cpp transformStack.hpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}scene
  ${AT3_TARGET_PREFIX}global
  )
target_include_directories( ${AT3_TARGET_PREFIX}scene PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
