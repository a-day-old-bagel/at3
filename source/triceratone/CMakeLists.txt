set( TARGET_NAME ${AT3_TARGET_PREFIX}triceratone )
set( TARGET_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR} )
add_subdirectory( "./ecs" )
add_executable( ${TARGET_NAME}
  main.cpp
  duneBuggy.cpp duneBuggy.h
  pyramid.cpp pyramid.h
  debugStuff.cpp debugStuff.h
  basicWalker.cpp basicWalker.h
  )
target_link_libraries( ${TARGET_NAME}
  ${TARGET_NAME}_systems
  ${BULLET_LIBRARIES}
  )
set_property( TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17 )
set_property( TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON )

#Copy assets to the build folder.
add_custom_command( TARGET ${TARGET_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/assets $<TARGET_FILE_DIR:${TARGET_NAME}>/assets )
