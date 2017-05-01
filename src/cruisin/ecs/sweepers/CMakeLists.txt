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