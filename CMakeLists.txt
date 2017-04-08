
cmake_minimum_required( VERSION 3.3 )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

project( at3 )

if(MSVC)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP12" )
else(MSVC)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif(MSVC)

# ARMADILLO
add_subdirectory( "./extern/armadillo-code" )

# None of the BELOW libs are stupid enough to modify their own source.
# This would have gone at the top, but armadillo had to be a jerk.
set( CMAKE_DISABLE_SOURCE_CHANGES ON )

# GLM
add_subdirectory( "./extern/glm" )

# ASSIMP
option( BUILD_SHARED_LIBS "Build package with shared libraries." ON )
option( ASSIMP_DOUBLE_PRECISION "Set to ON to enable double precision processing" OFF )
option( ASSIMP_OPT_BUILD_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF )
option( ASSIMP_ANDROID_JNIIOSYSTEM "Android JNI IOSystem support is active" OFF )
option( ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON )  # KEEP AN EYE ON THIS ONE
option( ASSIMP_BUILD_ZLIB "Build your own zlib" OFF )
option( ASSIMP_BUILD_ASSIMP_TOOLS "If the supplementary tools for Assimp are built in addition to the library." OFF )
option( ASSIMP_BUILD_SAMPLES "If the official samples are built as well (needs Glut)." OFF )
option( ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF )
option( ASSIMP_COVERALLS "Eańable this to measure test coverage." OFF )
add_subdirectory( "./extern/assimp" )
include_directories( "./extern/assimp/include" )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}/extern/assimp/include" )

# BULLET
add_subdirectory( "./extern/bullet3" )
include("./extern/bullet3/UseBullet.cmake")
include_directories( "./extern/bullet3/src" )

# STB
include_directories( "./extern/stb" )

# OPENGL
find_package( OpenGL REQUIRED )
include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )

# EPOXY
add_subdirectory( "./extern/libepoxy" )
include_directories( "./extern/libepoxy/include" )
include_directories( "./extern/libepoxy/registry" )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}/extern/libepoxy/include" )
#find_package( Epoxy REQUIRED )
#include_directories( SYSTEM epoxy_INCLUDE_DIR )

# SDL2
find_package( SDL2 REQUIRED )
include_directories( SYSTEM ${SDL2_INCLUDE_DIR} )

# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_LINK_TO_LIBS "glm" )
add_subdirectory( "./extern/ezecs" )

add_subdirectory( "./src" )