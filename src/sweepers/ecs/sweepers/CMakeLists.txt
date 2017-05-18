add_library( ${TARGET_NAME}_systems_sweepers STATIC
    CGenAlg.cpp
    CNeuralNet.cpp
    CParams.cpp
    utils.cpp
    )
target_include_directories( ${TARGET_NAME}_systems_sweepers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

#Copy ini to the build folder.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/params.ini ${TARGET_BIN_DIR}/params.ini COPYONLY )