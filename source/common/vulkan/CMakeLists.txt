
add_subdirectory( shaders )

add_library( ${AT3_TARGET_PREFIX}vulkan STATIC
  vkc.h
  vkcAlloc.h vkcAlloc.cpp
  vkcDataStore.h vkcDataStore.cpp
  vkcImplApi.h
  vkcImplInternalDynamic.h
  vkcImplInternalCallOnce.h
  vkcImplInternalResources.h
  vkcMaterial.h vkcMaterial.cpp
  vkcTypes.h
#  VulkanTexture.hpp
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
