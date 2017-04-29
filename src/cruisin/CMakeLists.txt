# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_LINK_TO_LIBS glm ${BULLET_LIBRARIES} )
add_subdirectory( "${CMAKE_SOURCE_DIR}/extern/ezecs" "${CMAKE_CURRENT_BINARY_DIR}/ecs" )

add_subdirectory( "./systems" )

add_executable( at3
    main.cpp
    duneBuggy.cpp
    )
target_link_libraries( at3
    at3_systems
    ${BULLET_LIBRARIES}
    )
set_property( TARGET at3 PROPERTY CXX_STANDARD 11 )
set_property( TARGET at3 PROPERTY CXX_STANDARD_REQUIRED ON )

#Copy assets to the build folder.
add_custom_command(TARGET at3 PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/assets $<TARGET_FILE_DIR:at3>/assets)
