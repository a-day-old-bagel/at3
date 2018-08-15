# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/components.hpp" )
set( EZECS_TARGET_PREFIX ${TARGET_NAME}_ )
set( EZECS_LINK_TO_LIBS ${AT3_TARGET_PREFIX}common )
add_subdirectory( "${EZECS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ezecs" )

add_library( ${TARGET_NAME}_components STATIC
  interface.cpp interface.hpp
  serialization.cpp serialization.hpp
  )
target_link_libraries( ${TARGET_NAME}_components
  ${AT3_TARGET_PREFIX}common
  ${EZECS_TARGET_PREFIX}ecs
  )
target_include_directories( ${TARGET_NAME}_components PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
