set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${epoxy_DEFINITIONS}")

add_subdirectory( global )
add_subdirectory( graphics )

add_library( common STATIC
    transform.cpp
    transformRAII.cpp
    transformStack.cpp
    )
target_link_libraries( common
    graphics
    assimp
    fastnoise
    )
target_include_directories( common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET common PROPERTY CXX_STANDARD 11 )
set_property( TARGET common PROPERTY CXX_STANDARD_REQUIRED ON )
