
# Setting some ground rules: Messes not allowed.
cmake_minimum_required( VERSION 3.9.2 FATAL_ERROR )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set( CMAKE_DISABLE_SOURCE_CHANGES ON )

# Use C++17 everywhere
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Name the project and set a prefix for naming internal targets and variables
project( at3 VERSION 0.01.01.01 LANGUAGES CXX C )
set( AT3_TARGET_PREFIX at3_ )

# Build and assemble third-party dependencies
add_subdirectory( "./extern" )

# Build this project's targets
add_subdirectory( "./source" )
