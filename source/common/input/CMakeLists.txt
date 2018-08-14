
add_library( ${AT3_TARGET_PREFIX}input STATIC
  sdlc.hpp sdlc.cpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}input
  ${AT3_TARGET_PREFIX}global
  )
target_include_directories( ${AT3_TARGET_PREFIX}input PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
