
add_library(graphics STATIC
    glUtil.cpp
    graphicsBackend.cpp
    loadedTexture.cpp
    shaderProgram.cpp
    shaders.cpp
    )
target_link_libraries(graphics
    global
    glm
#    assimp
    ${SDL2_LIBRARY}
    epoxy_static
    ${OPENGL_LIBRARIES}
#    fastnoise
    )
target_include_directories( graphics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )
set_property( TARGET graphics PROPERTY CXX_STANDARD 11 )
set_property( TARGET graphics PROPERTY CXX_STANDARD_REQUIRED ON )

# Convert shaders to C files
file( GLOB_RECURSE shaders
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tesc"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tese"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom"
    )
foreach( shader ${shaders} )
  file( RELATIVE_PATH shader_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${shader}" )
  string( REPLACE "/" "_" shader_target "${shader_rel}.c" )
  get_filename_component( shader_dir "${shader_rel}" DIRECTORY )
  message( STATUS "${CMAKE_CURRENT_SOURCE_DIR}/${shader_rel} ${CMAKE_CURRENT_BINARY_DIR}/${shader_target}" )
  add_custom_target( "${shader_target}"
      COMMAND xxd -include ${shader_rel} > "${CMAKE_CURRENT_BINARY_DIR}/${shader_target}"
      DEPENDS "${shader}" xxd_static
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      )
  add_dependencies( graphics
      "${shader_target}"
      )
endforeach()
