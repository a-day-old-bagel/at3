
cmake_minimum_required( VERSION 3.3 )
set( CMAKE_DISABLE_SOURCE_CHANGES ON )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project( at3 )

set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_LINK_TO_LIBS "glm" )
add_subdirectory( "./extern/ezecs" )

add_subdirectory( "./extern/glm" )

add_subdirectory( "./extern/assimp" )
include_directories( "./extern/assimp/include" )

add_subdirectory( "./extern/bullet3" )
include("./extern/bullet3/UseBullet.cmake")
include_directories( "./extern/bullet3/src" )

include_directories( "./extern/stb" )

find_package( OpenGL REQUIRED )
include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )

find_package( Epoxy REQUIRED )
include_directories( SYSTEM epoxy_INCLUDE_DIR )

find_package( SDL2 REQUIRED )
include_directories( SYSTEM ${SDL2_INCLUDE_DIR} )

add_subdirectory( "./src" )