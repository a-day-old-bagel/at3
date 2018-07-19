
add_library( ${TARGET_NAME}_mechanics STATIC
  cylinderMath.hpp cylinderMath.cpp
  )
target_link_libraries( ${TARGET_NAME}_mechanics
  ${AT3_TARGET_PREFIX}common
  )
target_include_directories( ${TARGET_NAME}_mechanics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
