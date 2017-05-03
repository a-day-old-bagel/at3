add_library( ${TARGET_NAME}_systems_sweepers STATIC
    C2DMatrix.cpp
    CGenAlg.cpp
    CMineSweeper.cpp
    CNeuralNet.cpp
    CParams.cpp
    sweepers.cpp
    utils.cpp
    )
target_include_directories( ${TARGET_NAME}_systems_sweepers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

#Copy ini to the build folder.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ecsDelegate.hpp ${EZECS_OUTPUT_DIR}/ecsDelegate.hpp COPYONLY )