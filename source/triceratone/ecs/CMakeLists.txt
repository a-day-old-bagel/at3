# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_TARGET_PREFIX ${TARGET_NAME}_ )
set( EZECS_LINK_TO_LIBS ${AT3_TARGET_PREFIX}common )
add_subdirectory( "${EZECS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ezecs" )

add_subdirectory( "./bullet" )
add_library( ${TARGET_NAME}_systems STATIC
  ecsInterface.cpp ecsInterface.h
  ecsSystem_controls.cpp ecsSystem_controls.h
  ecsSystem_animation.cpp ecsSystem_animation.h
  ecsSystem_physics.cpp ecsSystem_physics.h
  )
target_link_libraries( ${TARGET_NAME}_systems
  ${TARGET_NAME}_systems_bullet
  ${EZECS_TARGET_PREFIX}ecs
  )
target_include_directories( ${TARGET_NAME}_systems PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
