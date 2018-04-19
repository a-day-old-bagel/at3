
# These paths are used when compiling the shaders to SPIR-V and generating reflection files.
# Use -D options when building with CMake to specify non-default values, or use the GUI.
set( at3_glsl_dir_old "${CMAKE_CURRENT_SOURCE_DIR}/old" CACHE STRING "Where the GLSL shaders are stored" )
set( at3_glsl_dir "${CMAKE_CURRENT_SOURCE_DIR}/vulkan" CACHE STRING "Where the GLSL shaders are stored" )
set( at3_spv_dir "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Where to output the SPV shaders" )
set( at3_spv_c_dir "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE STRING "Where to output the SPV C shaders" )
set( at3_refl_dir "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Where to output the reflections" )
set( at3_conf_dir "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Where to output gpuInfo.conf" )



# Generate a Vulkan GPU configuration dump file
# This is the name of the configuration dump file
set( at3_gpuInfo_conf ${at3_conf_dir}/gpuInfo.conf )
# This points to the newly compiled glslangValidator
set( at3_glslangValidator $<TARGET_FILE:glslangValidator> )
# Get the configuration dump from glslangValidator
add_custom_command(
  OUTPUT ${at3_gpuInfo_conf}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${at3_conf_dir}/"
  COMMAND ${at3_glslangValidator} -c > ${at3_gpuInfo_conf}
  DEPENDS glslangValidator
)
# This target represents the completion of the task above
add_custom_target(
  at3_gpuInfo_conf
  DEPENDS ${at3_gpuInfo_conf}
)



# Compile each Vulkan GLSL source to SPV, then to a C header
message( STATUS "Will compile GLSL in ${at3_glsl_dir} to SPV in ${at3_spv_dir}" )
# Find the GLSL sources, put them in a list
file( GLOB_RECURSE at3_glsl_sources
  "${at3_glsl_dir}/*.frag"
  "${at3_glsl_dir}/*.vert"
  "${at3_glsl_dir}/*.tese"
  "${at3_glsl_dir}/*.tesc"
  "${at3_glsl_dir}/*.geom"
  "${at3_glsl_dir}/*.comp"
  )
foreach ( glsl_source ${at3_glsl_sources} )
  get_filename_component( glsl_file_name ${glsl_source} NAME )
  set( spv_output "${at3_spv_dir}/${glsl_file_name}.spv" )
  add_custom_command(
    OUTPUT ${spv_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${at3_spv_dir}/"
    COMMAND ${at3_glslangValidator} -V -o ${spv_output} ${glsl_source} ${at3_gpuInfo_conf}
    DEPENDS ${at3_glsl_sources} ${glsl_source} glslangValidator at3_gpuInfo_conf
  )
  list( APPEND at3_spv_binaries ${spv_output} )
  # Now to C
  set( spv_file_name "${glsl_file_name}.spv")
  set( c_output "${at3_spv_c_dir}/${spv_file_name}.c" )
  add_custom_command(
    OUTPUT ${c_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${at3_spv_c_dir}/"
    COMMAND xxd -include ${spv_file_name} > ${c_output}
    DEPENDS ${at3_glsl_sources} ${spv_output} xxd_static
    WORKING_DIRECTORY "${at3_spv_dir}"
  )
  list( APPEND at3_spv_c_headers ${c_output} )
endforeach ( glsl_source )
# This target represents the completion of the task above
add_custom_target(
  at3_glsl_to_spv_to_c
  DEPENDS ${at3_spv_binaries} ${at3_spv_c_headers}
)



# Compile the reflection program and Generate reflections of each spv binary.
# Add <experimental/filesystem> linker flags if necessary
if ( MSVC )
else ( MSVC )
  list( APPEND at3_extra_linker_flags stdc++fs )
endif ( MSVC )
# compile the stuff in ShaderPipeline
add_executable( ${AT3_TARGET_PREFIX}spvReflect spvReflect.cpp )
target_link_libraries( ${AT3_TARGET_PREFIX}spvReflect ${AT3_TARGET_PREFIX}external ${at3_extra_linker_flags} )
add_dependencies( ${AT3_TARGET_PREFIX}spvReflect at3_gpuInfo_conf at3_glsl_to_spv_to_c )
message( STATUS "Will compile SPV in ${at3_spv_dir} to REFL in ${at3_refl_dir}" )
set( spvReflect $<TARGET_FILE:${AT3_TARGET_PREFIX}spvReflect> )
# Make a list of the expected refl outputs
foreach ( glsl_source ${at3_glsl_sources} )
  get_filename_component( FILE_NAME ${glsl_source} NAME )
  set( refl_output "${at3_refl_dir}/${FILE_NAME}.refl" )
  list( APPEND at3_refl_files ${refl_output} )
endforeach ( glsl_source )
# Run the ShaderPipeline executable, if there's anything to do.
if ( at3_refl_files )
  add_custom_command(
    OUTPUT ${at3_refl_files}
    COMMAND ${spvReflect} ${at3_spv_dir} ${at3_refl_dir}
    DEPENDS ${at3_spv_sources} at3_glsl_to_spv_to_c ${AT3_TARGET_PREFIX}spvReflect
  )
endif ( at3_refl_files )
# This target represents the completion of the task above
add_custom_target(
  at3_spv_to_refl
  DEPENDS ${at3_refl_files}
)



# Compile each old non-vulkan GLSL source to C
message( STATUS "Will compile GLSL in ${at3_glsl_dir} to C in ${at3_spv_c_dir}" )
# Find the old GLSL sources, put them in a list
file( GLOB_RECURSE at3_old_glsl_sources
  "${at3_glsl_dir_old}/*.frag"
  "${at3_glsl_dir_old}/*.vert"
  "${at3_glsl_dir_old}/*.tese"
  "${at3_glsl_dir_old}/*.tesc"
  "${at3_glsl_dir_old}/*.geom"
  "${at3_glsl_dir_old}/*.comp"
  )
foreach ( glsl_source ${at3_old_glsl_sources} )
  get_filename_component( FILE_NAME ${glsl_source} NAME )
  set( c_output "${at3_spv_c_dir}/${FILE_NAME}.c" )
  add_custom_command(
    OUTPUT ${c_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${at3_spv_c_dir}/"
    COMMAND xxd -include ${FILE_NAME} > ${c_output}
    DEPENDS ${at3_old_glsl_sources} ${glsl_source} xxd_static
    WORKING_DIRECTORY "${at3_glsl_dir_old}"
  )
  list( APPEND at3_glsl_c_shaders ${c_output} )
endforeach ( glsl_source )
# This target represents the completion of the task above
add_custom_target(
  at3_glsl_to_c
  DEPENDS ${at3_glsl_c_shaders}
)



# This target signifies the completion of all shader compilation tasks
add_custom_target(
  ${AT3_TARGET_PREFIX}compile_shaders
  DEPENDS at3_glsl_to_c at3_glsl_to_spv_to_c at3_spv_to_refl
)

# Add an interface target to which to link in order to include the produced c files easily.
add_library( ${AT3_TARGET_PREFIX}include_shaders INTERFACE )
target_include_directories( ${AT3_TARGET_PREFIX}include_shaders INTERFACE ${at3_spv_c_dir} )
add_dependencies( ${AT3_TARGET_PREFIX}include_shaders at3_glsl_to_c )
