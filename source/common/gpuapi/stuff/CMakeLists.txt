
add_library( ${AT3_TARGET_PREFIX}vulkan STATIC
  camera.cpp camera.h
  config.h
  dataStore.h dataStore.cpp
  debug.h
  material_loading.h
  mesh_loading.cpp mesh_loading.h
  rendering.cpp rendering.h
  shader_inputs.h
  timing.h
  vkh.cpp vkh.h
  vkh_alloc.h
  vkh_initializers.h
  vkh_material.cpp vkh_material.h
  vkh_mesh.cpp vkh_mesh.h
  vkh_setup.h
  vkh_texture.h
  vkh_types.h
  os_sdl2.h state.h
  vulkanContext.h vulkanContext.cpp
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

add_executable( ${AT3_TARGET_PREFIX}vulkan_demo
  camera.cpp camera.h
  config.h
  dataStore.h dataStore.cpp
  debug.h
  main.cpp
  material_loading.h
  mesh_loading.cpp mesh_loading.h
  rendering.cpp rendering.h
  shader_inputs.h
  timing.h
  vkh.cpp vkh.h
  vkh_alloc.h
  vkh_initializers.h
  vkh_material.cpp vkh_material.h
  vkh_mesh.cpp vkh_mesh.h
  vkh_setup.h
  vkh_texture.h
  vkh_types.h
  os_sdl2.h state.h
#  vulkanContext.h vulkanContext.cpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}vulkan_demo
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}global
  ${AT3_TARGET_PREFIX}include_shaders
)
add_dependencies( ${AT3_TARGET_PREFIX}vulkan_demo
  ${AT3_TARGET_PREFIX}compile_shaders
)
add_custom_command(
  TARGET ${AT3_TARGET_PREFIX}vulkan_demo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  /home/volundr/VkBindingBenchmark/data/mesh
  $<TARGET_FILE_DIR:${AT3_TARGET_PREFIX}vulkan_demo>/meshes
)
add_custom_command(
  TARGET ${AT3_TARGET_PREFIX}vulkan_demo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_BINARY_DIR}/source/common/gpuapi/shaders
  $<TARGET_FILE_DIR:${AT3_TARGET_PREFIX}vulkan_demo>/shaders
)
