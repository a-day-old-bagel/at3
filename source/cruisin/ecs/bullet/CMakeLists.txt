add_library( ${TARGET_NAME}_systems_bullet STATIC
  bulletDebug.cpp bulletDebug.h
  )
target_link_libraries( ${TARGET_NAME}_systems_bullet
  ${AT3_TARGET_PREFIX}common
  ${BULLET_LIBRARIES}
  )
target_include_directories( ${TARGET_NAME}_systems_bullet PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
