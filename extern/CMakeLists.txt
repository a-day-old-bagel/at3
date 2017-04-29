add_library( fastnoise STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise/FastNoise.cpp
    )
target_include_directories( fastnoise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise
    )