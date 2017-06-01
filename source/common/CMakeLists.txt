set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${epoxy_DEFINITIONS}")

add_subdirectory( global )
add_subdirectory( graphics )

add_library( ${AT3_TARGET_PREFIX}common STATIC
    transform.cpp
    transformRAII.cpp
    transformStack.cpp
    )
target_link_libraries( ${AT3_TARGET_PREFIX}common
    ${AT3_TARGET_PREFIX}graphics
    assimp
    fastnoise
    )
target_include_directories( ${AT3_TARGET_PREFIX}common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET ${AT3_TARGET_PREFIX}common PROPERTY CXX_STANDARD 11 )
set_property( TARGET ${AT3_TARGET_PREFIX}common PROPERTY CXX_STANDARD_REQUIRED ON )
