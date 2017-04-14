
cmake_minimum_required( VERSION 3.3 )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
# Armadillo generates a makefile in-source, and I'm tired of dealing with crap like that, so I just disabled this.
# set( CMAKE_DISABLE_SOURCE_CHANGES ON )

project( at3 )

if( MSVC )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP12 /std:c++14" ) # /Gm not good with /MP ?
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /incremental /debug:fastlink" )
  add_definitions( -DARMA_USE_CXX11 )
else( MSVC )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif( MSVC )

# ARMADILLO
add_subdirectory( "./extern/armadillo-code" )

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

# GLM
add_subdirectory( "./extern/glm" )

# BULLET
option( USE_DOUBLE_PRECISION "Use double precision"	OFF )
option( USE_GRAPHICAL_BENCHMARK "Use Graphical Benchmark" ON )
option( BUILD_SHARED_LIBS "Use shared libraries" ON )                                             # CHANGED
option( USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD "Use btSoftMultiBodyDynamicsWorld" OFF )
option( BULLET2_USE_THREAD_LOCKS "Build Bullet 2 libraries with mutex locking around certain operations" OFF )
option( USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF )
option( USE_CUSTOM_VECTOR_MATH "Use custom vectormath library" OFF )
option( USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library DLL (/MD or /MDd)" ON )
option( USE_MSVC_SSE "Use MSVC /arch:sse option"	ON )
option( USE_MSVC_SSE2 "Compile your program with SSE2 instructions" ON )
OPTION( BUILD_CPU_DEMOS "Build original Bullet CPU examples" OFF )                                # CHANGED
OPTION( USE_GLUT "Use Glut" OFF )                                                                 # CHANGED
OPTION( BUILD_OPENGL3_DEMOS "Set when you want to build Bullet 3 OpenGL3+ demos" OFF )            # CHANGED
OPTION( BUILD_BULLET2_DEMOS "Set when you want to build the Bullet 2 demos" OFF )                 # CHANGED
OPTION( BUILD_EXTRAS "Set when you want to build the extras" OFF)                                 # CHANGED
OPTION( INSTALL_LIBS "Set when you want to install libraries" OFF )                               # CHANGED
OPTION( INSTALL_EXTRA_LIBS "Set when you want extra libraries installed" OFF )
OPTION( BUILD_UNIT_TESTS "Build Unit Tests"	ON )
add_subdirectory( "./extern/bullet3" )
include( "${CMAKE_CURRENT_BINARY_DIR}/extern/bullet3/BulletConfig.cmake" )
include( "./extern/bullet3/UseBullet.cmake" )
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

# SDL2
find_package( SDL2 REQUIRED )
include_directories( SYSTEM ${SDL2_INCLUDE_DIR} )

# XXD
option( XXD_STATIC_LIB "Build a static lib of xxd" ON )
option( XXD_EXECUTABLE "Build xxd as an executable" OFF )
add_subdirectory( "./extern/xxd" )

# EZECS
set( EZECS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ecsConfig.hpp" )
set( EZECS_LINK_TO_LIBS glm ${BULLET_LIBRARIES} armadillo) # BulletDynamics
add_subdirectory( "./extern/ezecs" )

add_subdirectory( "./src" )