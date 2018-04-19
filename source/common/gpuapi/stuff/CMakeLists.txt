
add_executable( ${AT3_TARGET_PREFIX}vulkan
  camera.cpp camera.h
  Common.h
  config.h
  debug.h
  main.cpp
  material_loading.h
  mesh_loading.cpp mesh_loading.h
  null_store.cpp null_store.h
  rendering.cpp rendering.h
  shader_inputs.h
  ssbo_store.cpp ssbo_store.h
  timing.h
  ubo_store.cpp ubo_store.h
  vkh.cpp vkh.h
  vkh_alloc.h
  vkh_initializers.h
  vkh_material.cpp vkh_material.h
  vkh_mesh.cpp vkh_mesh.h
  vkh_setup.h
  vkh_texture.h
  vkh_types.h
  os_sdl2.h state.h )
target_link_libraries( ${AT3_TARGET_PREFIX}vulkan
  ${AT3_TARGET_PREFIX}external
  ${AT3_TARGET_PREFIX}include_shaders
)
add_dependencies( ${AT3_TARGET_PREFIX}vulkan
  ${AT3_TARGET_PREFIX}compile_shaders
)
add_custom_command(
  TARGET ${AT3_TARGET_PREFIX}vulkan POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  /home/volundr/VkBindingBenchmark/data/mesh
  $<TARGET_FILE_DIR:${AT3_TARGET_PREFIX}vulkan>/meshes
)
add_custom_command(
  TARGET ${AT3_TARGET_PREFIX}vulkan POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_BINARY_DIR}/source/common/gpuapi/shaders
  $<TARGET_FILE_DIR:${AT3_TARGET_PREFIX}vulkan>/shaders
)
