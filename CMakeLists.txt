
cmake_minimum_required( VERSION 3.7 FATAL_ERROR)
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set( CMAKE_DISABLE_SOURCE_CHANGES ON )

# By default, at3 will build some external dependencies (like assimp or bullet) from included sources instead of
# praying that cmake can actually find them correctly on the user's system without extra toil.
option( AT3_USE_INCLUDED_LIBS
        "build external dependencies instead of finding them on your system (except opengl and sdl)"
        ON )

# Some global compiler flags are set here. This is not a great way to do this, but it fixes things for now.
if( MSVC )
  set( AT3_MSVC_RELEASE_FLAGS "/MD /MP12 /std:c++14" )
  set( AT3_MSVC_DEBUG_FLAGS "/MDd /MP12 /std:c++14" )

  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT} ${AT3_MSVC_RELEASE_FLAGS}" )

  set( CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${AT3_MSVC_DEBUG_FLAGS}" )
  set( CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS_DEBUG_INIT} ${AT3_MSVC_DEBUG_FLAGS}" )

  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /incremental /debug:fastlink" )
else( MSVC )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif( MSVC )

if (AT3_USE_INCLUDED_LIBS) # BUILD AND INCLUDE EXTERNS

  # ASSIMP
  option( BUILD_SHARED_LIBS "Build package with shared libraries." OFF )
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
  option( BUILD_SHARED_LIBS "Use shared libraries" OFF )                                            # CHANGED
  option( USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD "Use btSoftMultiBodyDynamicsWorld" OFF )
  option( BULLET2_USE_THREAD_LOCKS "Build Bullet 2 libraries with mutex locking around certain operations" OFF )
  option( USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF )
  option( USE_CUSTOM_VECTOR_MATH "Use custom vectormath library" OFF )
  option( USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library DLL (/MD or /MDd)" ON )
  option( USE_MSVC_SSE "Use MSVC /arch:sse option"	ON )
  option( USE_MSVC_SSE2 "Compile your program with SSE2 instructions" ON )
  option( BUILD_CPU_DEMOS "Build original Bullet CPU examples" OFF )                                # CHANGED
  option( USE_GLUT "Use Glut" OFF )                                                                 # CHANGED
  option( BUILD_OPENGL3_DEMOS "Set when you want to build Bullet 3 OpenGL3+ demos" OFF )            # CHANGED
  option( BUILD_BULLET2_DEMOS "Set when you want to build the Bullet 2 demos" OFF )                 # CHANGED
  option( BUILD_EXTRAS "Set when you want to build the extras" OFF)                                 # CHANGED
  option( INSTALL_LIBS "Set when you want to install libraries" OFF )                               # CHANGED
  option( INSTALL_EXTRA_LIBS "Set when you want extra libraries installed" OFF )
  option( BUILD_UNIT_TESTS "Build Unit Tests"	OFF )
  add_subdirectory( "./extern/bullet3" )
  include( "${CMAKE_CURRENT_BINARY_DIR}/extern/bullet3/BulletConfig.cmake" )
  include( "./extern/bullet3/UseBullet.cmake" )
  include_directories( "./extern/bullet3/src" )

  # EPOXY
  add_subdirectory( "./extern/libepoxy" )
  include_directories( "./extern/libepoxy/include" )
  include_directories( "./extern/libepoxy/registry" )
  include_directories( "${CMAKE_CURRENT_BINARY_DIR}/extern/libepoxy/include" )

else (AT3_USE_INCLUDED_LIBS)
  message(FATAL_ERROR
          "Use of system libraries (instead of building the included ones) is not yet supported.\
          Set option AT3_USE_INCLUDED_LIBS to ON.")
endif (AT3_USE_INCLUDED_LIBS)

# This is way down here in order to suppress CMP0048 warnings by the above included external sources.
# I dare you to find a better way to fix this that doesn't require suppressing ALL dev warnings (-Wno-dev)
# The warnings are generated because I use "VERSION <number>" in the project command, and the above projects don't.
project( at3 VERSION 0.01.01.01 LANGUAGES CXX)

# These libraries will need to be built from source in all cases
  # XXD
  option( XXD_STATIC_LIB "Build a static lib of xxd" ON )
  option( XXD_EXECUTABLE "Build xxd as an executable" OFF )
  add_subdirectory( "./extern/xxd" )
  # FASTNOISE, ETC.
  add_subdirectory( "./extern")

# OpenGL and SDL2 are not included in source, unlike other external libraries, so they will always need to be found.
  find_package( OpenGL REQUIRED )
  include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )
  find_package( SDL2 REQUIRED )
  include_directories( SYSTEM ${SDL2_INCLUDE_DIR} )
  # stb image library is header only, and will always be included like this
  include_directories( "./extern/stb" )

add_subdirectory( "./src" )