
# Setting some ground rules: Messes not allowed.
cmake_minimum_required( VERSION 3.6 FATAL_ERROR )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set( CMAKE_DISABLE_SOURCE_CHANGES ON )

# Build third-party libraries that are already set up for cmake
# This call works best before the "project" call is made.
include( cmake/buildThirdParty.cmake )

# Define project name, version, language, and target name prefix
project( at3 VERSION 0.01.01.01 LANGUAGES CXX C )
set( AT3_TARGET_PREFIX at3_ )

# Include third-party libraries that the user has installed on their system
include( cmake/findSystemLibs.cmake )

# Build third-party libraries that required custom targets to be made
add_subdirectory( "./extern" )

# Build this project's targets
add_subdirectory( "./source" )
