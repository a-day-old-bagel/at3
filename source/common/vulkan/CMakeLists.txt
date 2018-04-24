
add_subdirectory( shaders )

add_library( ${AT3_TARGET_PREFIX}vulkan STATIC
  dataStore.h
  debug.h
  material_loading.h
  mesh_loading.h
  rendering.cpp rendering.h
  shader_inputs.h
  timing.h
  vkh.cpp vkh.h
  vkh_alloc.cpp vkh_alloc.h
  vkh_initializers.h
  vkh_material.cpp vkh_material.h
  vkh_mesh.cpp vkh_mesh.h
  vkh_setup.cpp vkh_setup.h
  vkh_texture.h
  vkh_types.h
  vulkanContext.h
  )
target_link_libraries( ${AT3_TARGET_PREFIX}vulkan
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}global
  ${AT3_TARGET_PREFIX}include_shaders
  )
target_include_directories( ${AT3_TARGET_PREFIX}vulkan PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
add_dependencies( ${AT3_TARGET_PREFIX}vulkan
  ${AT3_TARGET_PREFIX}compile_shaders
  )
