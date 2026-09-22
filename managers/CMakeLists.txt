add_library(VisionFlowManagers STATIC
    flowcv_manager.h
    flowcv_manager.cpp
    node_manager.h
    node_manager.cpp
    plugin_manager.h
    plugin_manager.cpp
)

add_library(VisionFlow::Managers ALIAS VisionFlowManagers)

target_include_directories(VisionFlowManagers
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowManagers
    PUBLIC
        VisionFlow::Sdk
    PRIVATE
        VisionFlow::Utils
)
