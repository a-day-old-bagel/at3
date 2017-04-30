## EZECS
#set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
#set( EZECS_TARGET_PREFIX ${TARGET_NAME} )
#set( EZECS_LINK_TO_LIBS glm ${BULLET_LIBRARIES} )
#add_subdirectory( "${CMAKE_SOURCE_DIR}/extern/ezecs" "${CMAKE_CURRENT_BINARY_DIR}/ezecs" )

add_library( ${TARGET_NAME}_systems STATIC
    ecsSystem_controls.cpp
    ecsSystem_movement.cpp
    ecsSystem_physics.cpp
    bulletDebug.cpp
    )
target_link_libraries( ${TARGET_NAME}_systems
    common
    ${EZECS_TARGET_PREFIX}ecs_static
    ${BULLET_LIBRARIES}
    )
target_include_directories( ${TARGET_NAME}_systems PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET ${TARGET_NAME}_systems PROPERTY CXX_STANDARD 11 )
set_property( TARGET ${TARGET_NAME}_systems PROPERTY CXX_STANDARD_REQUIRED ON )