add_subdirectory("./common")
add_subdirectory("./systems")
add_executable( at3
    main.cpp
    )
target_link_libraries( at3
    at3_systems
    )
set_property( TARGET at3 PROPERTY CXX_STANDARD 11 )
set_property( TARGET at3 PROPERTY CXX_STANDARD_REQUIRED ON )

#Copy assets to the build folder.
add_custom_command(TARGET at3 PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:at3>/assets)
