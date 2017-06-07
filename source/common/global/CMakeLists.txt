
add_library(${AT3_TARGET_PREFIX}global STATIC
  settings.cpp
  )
target_link_libraries(${AT3_TARGET_PREFIX}global
  SDL_vulkan
  )
target_include_directories(${AT3_TARGET_PREFIX}global PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
set_property(TARGET ${AT3_TARGET_PREFIX}global PROPERTY CXX_STANDARD 11)
set_property(TARGET ${AT3_TARGET_PREFIX}global PROPERTY CXX_STANDARD_REQUIRED ON)
