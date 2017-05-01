# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_TARGET_PREFIX ${TARGET_NAME}_ )
set( EZECS_LINK_TO_LIBS glm ${BULLET_LIBRARIES} ${TARGET_NAME}_systems_sweepers )
add_subdirectory( "${CMAKE_SOURCE_DIR}/extern/ezecs" "${CMAKE_CURRENT_BINARY_DIR}/ezecs" )

add_subdirectory( "./bullet" )
add_subdirectory( "./sweepers" )
add_library( ${TARGET_NAME}_systems STATIC
    dualityInterface.cpp
    ecsSystem_controls.cpp
    ecsSystem_movement.cpp
    ecsSystem_physics.cpp
    ecsSystem_ai.cpp
    )
target_link_libraries( ${TARGET_NAME}_systems
    ${EZECS_TARGET_PREFIX}ecs_static
    ${TARGET_NAME}_systems_sweepers
    ${TARGET_NAME}_systems_bullet
    )
target_include_directories( ${TARGET_NAME}_systems PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET ${TARGET_NAME}_systems PROPERTY CXX_STANDARD 11 )
set_property( TARGET ${TARGET_NAME}_systems PROPERTY CXX_STANDARD_REQUIRED ON )