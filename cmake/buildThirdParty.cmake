
# Some global compiler flags are set here. This is not a great way to do this, but it fixes things for now.
# FIXME: Find some other way to make the third-party library compilations behave
if ( MSVC )
  set( AT3_MSVC_RELEASE_FLAGS "/MD /MP12 /std:c++14" )
  set( AT3_MSVC_DEBUG_FLAGS "/MDd /MP12 /std:c++14" )

  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AT3_MSVC_RELEASE_FLAGS}" )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT} ${AT3_MSVC_RELEASE_FLAGS}" )

  set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${AT3_MSVC_DEBUG_FLAGS}" )
  set( CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS_DEBUG_INIT} ${AT3_MSVC_DEBUG_FLAGS}" )

  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /incremental /debug:fastlink" )
else ( MSVC )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif ( MSVC )

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
option( USE_DOUBLE_PRECISION "Use double precision" OFF )
option( USE_GRAPHICAL_BENCHMARK "Use Graphical Benchmark" ON )
option( BUILD_SHARED_LIBS "Use shared libraries" OFF )                                            # CHANGED
option( USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD "Use btSoftMultiBodyDynamicsWorld" OFF )
option( BULLET2_USE_THREAD_LOCKS "Build Bullet 2 libraries with mutex locking around certain operations" OFF )
option( USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF )
option( USE_CUSTOM_VECTOR_MATH "Use custom vectormath library" OFF )
option( USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library DLL (/MD or /MDd)" ON )
option( USE_MSVC_SSE "Use MSVC /arch:sse option" ON )
option( USE_MSVC_SSE2 "Compile your program with SSE2 instructions" ON )
option( BUILD_CPU_DEMOS "Build original Bullet CPU examples" OFF )                                # CHANGED
option( USE_GLUT "Use Glut" OFF )                                                                 # CHANGED
option( BUILD_OPENGL3_DEMOS "Set when you want to build Bullet 3 OpenGL3+ demos" OFF )            # CHANGED
option( BUILD_BULLET2_DEMOS "Set when you want to build the Bullet 2 demos" OFF )                 # CHANGED
option( BUILD_EXTRAS "Set when you want to build the extras" OFF )                                 # CHANGED
option( INSTALL_LIBS "Set when you want to install libraries" OFF )                               # CHANGED
option( INSTALL_EXTRA_LIBS "Set when you want extra libraries installed" OFF )
option( BUILD_UNIT_TESTS "Build Unit Tests" OFF )
add_subdirectory( "./extern/bullet3" )
include( "${CMAKE_CURRENT_BINARY_DIR}/extern/bullet3/BulletConfig.cmake" )
include( "./extern/bullet3/UseBullet.cmake" )
include_directories( "./extern/bullet3/src" )

# EPOXY
add_subdirectory( "./extern/libepoxy" )
include_directories( "./extern/libepoxy/include" )
include_directories( "./extern/libepoxy/registry" )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}/extern/libepoxy/include" )

# SDL_VULKAN
add_subdirectory( "./extern/sdl_vulkan" )

# XXD
option( XXD_STATIC_LIB "Build a static lib of xxd" ON )
option( XXD_EXECUTABLE "Build xxd as an executable" OFF )
add_subdirectory( "./extern/xxd" )

# stb image library is header-only
include_directories( "./extern/stb" )

# Runtime Compiled C++ stuff
#TODO
