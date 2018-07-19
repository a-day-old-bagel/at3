
add_library( ${AT3_TARGET_PREFIX}network STATIC
  client.hpp client.cpp
  netRole.hpp netRole.cpp
  server.hpp server.cpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}network
  ${AT3_TARGET_PREFIX}external
  )
target_include_directories( ${AT3_TARGET_PREFIX}network PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
