
add_library( ${AT3_TARGET_PREFIX}network STATIC
  client.hpp client.cpp
  netInterface.hpp netInterface.cpp
  server.hpp server.cpp
  userPacketEnums.hpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}network
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}global
  )
target_include_directories( ${AT3_TARGET_PREFIX}network PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
