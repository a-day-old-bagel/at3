add_library( fastnoise STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise/FastNoise.cpp
    )
#target_link_libraries( fastnoise ${})
target_include_directories( fastnoise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise
    )
#set_property( TARGET fastnoise PROPERTY CXX_STANDARD 11 )
#set_property( TARGET fastnoise PROPERTY CXX_STANDARD_REQUIRED ON )