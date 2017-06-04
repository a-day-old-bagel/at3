
# OpenGL and SDL2/GLFW are not included, unlike other external libraries, so they will always need to be found.
find_package( OpenGL REQUIRED )
include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )
find_package( SDL2 REQUIRED )
include_directories( SYSTEM ${SDL2_INCLUDE_DIR} )