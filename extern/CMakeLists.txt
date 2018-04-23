
project( ${AT3_TARGET_PREFIX}extern LANGUAGES CXX C )

# helpful message
message( "To remove a submodule for real, see https://stackoverflow.com/a/36593218" )


# Find the libraries that should be installed on the user's system
# In Windows, this will require downloading the SDL2 Development Libraries and installing the Vulkan SDK,
# and making sure your environment variables are set up to point to them correctly

# OPENGL
find_package( OpenGL REQUIRED )
list( APPEND ${AT3_TARGET_PREFIX}external_includes
  ${OPENGL_INCLUDE_DIR}
  )

# EPOXY
add_subdirectory( libepoxy )
list( APPEND ${AT3_TARGET_PREFIX}external_includes
  ./libepoxy/include
  ./libepoxy/registry
  ${CMAKE_CURRENT_BINARY_DIR}/libepoxy/include
  )
list( APPEND ${AT3_TARGET_PREFIX}extra_definitions
  ${epoxy_DEFINITIONS}
  )

# SDL_VULKAN
add_subdirectory( sdl_vulkan )

# ASSIMP
option( BUILD_SHARED_LIBS "Build package with shared libraries." OFF )
option( BUILD_FRAMEWORK "Build package as Mac OS X Framework bundle." OFF )
option( ASSIMP_DOUBLE_PRECISION "Set to ON to enable double precision processing" OFF )
option( ASSIMP_OPT_BUILD_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF )
option( ASSIMP_ANDROID_JNIIOSYSTEM "Android JNI IOSystem support is active" OFF )
option( ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON )
option( ASSIMP_BUILD_ZLIB "Build your own zlib" OFF )
option( ASSIMP_BUILD_ASSIMP_TOOLS "If the supplementary tools for Assimp are built in addition to the library." OFF )
option( ASSIMP_BUILD_SAMPLES "If the official samples are built as well (needs Glut)." OFF )
option( ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF )
option( ASSIMP_COVERALLS "Enable this to measure test coverage." OFF )
option( ASSIMP_WERROR "Treat warnings as errors." OFF )
option( ASSIMP_ASAN "Enable AddressSanitizer." OFF )
option( ASSIMP_UBSAN "Enable Undefined Behavior sanitizer." OFF )
option( SYSTEM_IRRXML "Use system installed Irrlicht/IrrXML library." OFF )
option( BUILD_DOCS "Build documentation using Doxygen." OFF )
option( ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_IMPORTER value" FALSE )
option( ASSIMP_BUILD_OBJ_IMPORTER "build obj importer" TRUE )
option( ASSIMP_BUILD_COLLADA_IMPORTER "build collada (dae) importer" TRUE )
add_subdirectory( assimp )
list( APPEND ${AT3_TARGET_PREFIX}external_includes
  ${CMAKE_CURRENT_BINARY_DIR}/assimp/include
  )

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
add_subdirectory( bullet3 )
list( APPEND ${AT3_TARGET_PREFIX}bullet_libs  # TODO: Use Bullet 3 libs instead
  BulletSoftBody
  BulletDynamics
  BulletCollision
  BulletInverseDynamics
  LinearMath
  )
list( APPEND ${AT3_TARGET_PREFIX}external_includes
  ./bullet3/src
  )

# SPIRV_CROSS
option( SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS "Instead of throwing exceptions assert" ON )
add_subdirectory( spirv-cross )

# GLSLANG
option( SKIP_GLSLANG_INSTALL "Skip installation" ON )
option( ENABLE_AMD_EXTENSIONS "Enables support of AMD-specific extensions" ON )
option( ENABLE_GLSLANG_BINARIES "Builds glslangValidator and spirv-remap" ON )
option( ENABLE_NV_EXTENSIONS "Enables support of Nvidia-specific extensions" ON )
option( ENABLE_HLSL "Enables HLSL input support" OFF )
option( ENABLE_OPT "Enables spirv-opt capability if present" ON )
add_subdirectory( glslang )

# XXD
option( XXD_STATIC_LIB "Build a static lib of xxd" ON )
option( XXD_EXECUTABLE "Build xxd as an executable" OFF )
add_subdirectory( xxd )

# FASTNOISE
add_library( fastnoise STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise/FastNoise.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise/FastNoise.h
  )
target_include_directories( fastnoise PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/fastnoise
  )

# TODO: Look again at runtime compiled C++

# Add <experimental/filesystem> and other linker flags if necessary
if ( MSVC )
else ( MSVC )
  list( APPEND ${AT3_TARGET_PREFIX}extra_linker_flags stdc++fs )
endif ( MSVC )

# Assemble the external dependencies together into a linkable interface that also automatically includes all
# the necessary directories
add_library( ${AT3_TARGET_PREFIX}external INTERFACE )
target_include_directories( ${AT3_TARGET_PREFIX}external INTERFACE SYSTEM
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include
  ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include
  ${CMAKE_CURRENT_SOURCE_DIR}/stb
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
  ${${AT3_TARGET_PREFIX}external_includes}
  )
target_link_libraries( ${AT3_TARGET_PREFIX}external INTERFACE
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${Vulkan_LIBRARY}
  ${XCB_LIBS}
  SDL_vulkan
  epoxy_static
  assimp
  ${${AT3_TARGET_PREFIX}bullet_libs}
  spirv-cross-glsl
  fastnoise
  ${${AT3_TARGET_PREFIX}extra_linker_flags}
  )
target_compile_definitions( ${AT3_TARGET_PREFIX}external INTERFACE
  ${${AT3_TARGET_PREFIX}extra_definitions}
  )
