
add_library( ${TARGET_NAME}_systems STATIC
  scene.cpp scene.hpp
  controls.cpp controls.hpp
  network.cpp network.hpp
  physics.cpp physics.hpp
  )
target_link_libraries( ${TARGET_NAME}_systems
  ${AT3_TARGET_PREFIX}common
  ${TARGET_NAME}_constructs
  )
target_include_directories( ${TARGET_NAME}_systems PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
