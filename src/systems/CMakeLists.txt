add_library( at3_systems STATIC
    ecsSystem_controls.cpp
    ecsSystem_kalman.cpp
    ecsSystem_movement.cpp
    ecsSystem_physics.cpp
    bulletDebug.cpp
    )
target_link_libraries( at3_systems
    ecs_static
    common
    BulletDynamics BulletCollision LinearMath
    armadillo
    )
target_include_directories( at3_systems PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
set_property( TARGET at3_systems PROPERTY CXX_STANDARD 11 )
set_property( TARGET at3_systems PROPERTY CXX_STANDARD_REQUIRED ON )