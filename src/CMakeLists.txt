# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_TARGET_PREFIX ${CMAKE_PROJECT_NAME}_ )
set( EZECS_LINK_TO_LIBS glm ${BULLET_LIBRARIES} )
add_subdirectory( "${CMAKE_SOURCE_DIR}/extern/ezecs" "${CMAKE_CURRENT_BINARY_DIR}/ezecs" )

add_subdirectory("./common")
add_subdirectory("./cruisin")