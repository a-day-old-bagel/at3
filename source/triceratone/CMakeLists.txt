set( TARGET_NAME ${AT3_TARGET_PREFIX}triceratone )
set( TARGET_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR} )
add_subdirectory( "./ecs" )
add_executable( ${TARGET_NAME}
  main.cpp
  duneBuggy.cpp duneBuggy.h
  duneBuggyVk.cpp duneBuggyVk.h
  pyramid.cpp pyramid.h
  pyramidVk.cpp pyramidVk.h
  debugStuff.cpp debugStuff.h
  basicWalker.cpp basicWalker.h
  basicWalkerVk.cpp basicWalkerVk.h
  )
target_link_libraries( ${TARGET_NAME}
  ${TARGET_NAME}_systems
  )

#Copy assets to the build folder.
add_custom_command( TARGET ${TARGET_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${TARGET_NAME}>/assets )

if ( WIN32 )

  # Test 32/64 bits, copy dll's to the build folder
  string( REGEX REPLACE "\\\\" "/" correctDllName "$ENV{SDL2}/lib/x64/SDL2.dll" )
  if ( "${CMAKE_SIZEOF_VOID_P}" EQUAL "8" )
    message( STATUS "Target is x64" )
    configure_file( ${correctDllName} ${CMAKE_CURRENT_BINARY_DIR}/SDL2.dll COPYONLY )
  else ( "${CMAKE_SIZEOF_VOID_P}" EQUAL "8" )
    message( STATUS "Target is x86" )
    configure_file( ${correctDllName} ${CMAKE_CURRENT_BINARY_DIR}/SDL2.dll COPYONLY )
  endif ( "${CMAKE_SIZEOF_VOID_P}" EQUAL "8" )

  # Test Release/Debug mode, copy SDLvulkan dll to build folder
  if ( CMAKE_BUILD_TYPE MATCHES DEBUG )
    add_custom_command( TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/lib/SDL_vulkand.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/SDL_vulkand.dll
      )
  else()
    add_custom_command( TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/lib/SDL_vulkan.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/SDL_vulkan.dll
      )
  endif()

endif ( WIN32 )
