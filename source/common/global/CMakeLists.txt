
add_library(global STATIC
    settings.cpp
    )
target_link_libraries(global

    )
target_include_directories( global PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET global PROPERTY CXX_STANDARD 11 )
set_property( TARGET global PROPERTY CXX_STANDARD_REQUIRED ON )
