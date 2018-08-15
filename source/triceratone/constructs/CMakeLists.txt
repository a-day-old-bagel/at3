
add_library( ${TARGET_NAME}_constructs STATIC
  duneBuggy.cpp duneBuggy.hpp
  freeCam.cpp freeCam.hpp
  playerSet.cpp playerSet.hpp
  pyramid.cpp pyramid.hpp
  walker.cpp walker.hpp
  )
target_link_libraries( ${TARGET_NAME}_constructs
  ${AT3_TARGET_PREFIX}common
  ${TARGET_NAME}_mechanics
  ${TARGET_NAME}_components
  )
target_include_directories( ${TARGET_NAME}_constructs PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
