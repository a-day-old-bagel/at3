
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

add_library( ${AT3_TARGET_PREFIX}graphics STATIC
  graphicsBackend.cpp graphicsBackend.h
  loadedTexture.cpp loadedTexture.h
  material.cpp material.h
  openglCalls.cpp
  openglValidation.cpp openglValidation.h
  sdl2Calls.cpp
  shaderProgram.cpp shaderProgram.h
  shaders.cpp shaders.h
  tiny_obj_loader.h
  vulkanCalls.cpp
  vulkanBackend.cpp vulkanBackend.h
  )
target_link_libraries( ${AT3_TARGET_PREFIX}graphics
  ${AT3_TARGET_PREFIX}global
  glm
  SDL_vulkan
  epoxy_static
  ${OPENGL_LIBRARIES}
  realtimeutils
  #fastnoise
  )
target_include_directories( ${AT3_TARGET_PREFIX}graphics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
set_property( TARGET ${AT3_TARGET_PREFIX}graphics PROPERTY CXX_STANDARD 11 )
set_property( TARGET ${AT3_TARGET_PREFIX}graphics PROPERTY CXX_STANDARD_REQUIRED ON )

# Convert shaders to C files
file( GLOB_RECURSE shaders
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tesc"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tese"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom"
  )
foreach ( shader ${shaders} )
  file( RELATIVE_PATH shader_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${shader}" )
  string( REPLACE "/" "_" shader_target "${shader_rel}.c" )
  get_filename_component( shader_dir "${shader_rel}" DIRECTORY )
  message( STATUS "${CMAKE_CURRENT_SOURCE_DIR}/${shader_rel} ${CMAKE_CURRENT_BINARY_DIR}/${shader_target}" )
  add_custom_target( "${AT3_TARGET_PREFIX}${shader_target}"
    COMMAND xxd -include ${shader_rel} > "${CMAKE_CURRENT_BINARY_DIR}/${shader_target}"
    DEPENDS "${shader}" xxd_static
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  add_dependencies( ${AT3_TARGET_PREFIX}graphics
    "${AT3_TARGET_PREFIX}${shader_target}"
    )
endforeach ()
