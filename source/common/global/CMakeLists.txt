
add_library( ${AT3_TARGET_PREFIX}global STATIC
  definitions.hpp
  fileSystemHelpers.cpp fileSystemHelpers.hpp
  macros.hpp
  math.hpp math.cpp
  settings.cpp settings.hpp
  TODO.hpp
  )
target_link_libraries( ${AT3_TARGET_PREFIX}global
  ${AT3_TARGET_PREFIX}external
  )
target_include_directories( ${AT3_TARGET_PREFIX}global PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
