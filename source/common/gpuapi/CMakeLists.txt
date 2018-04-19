
add_subdirectory( shaders )
add_subdirectory( stuff )

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
  vulkanContext.h vulkanContext.cpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}graphics
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}global
  ${AT3_TARGET_PREFIX}include_shaders
  )
target_include_directories( ${AT3_TARGET_PREFIX}graphics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/shaders
  )
add_dependencies( ${AT3_TARGET_PREFIX}graphics
  ${AT3_TARGET_PREFIX}compile_shaders
  )
