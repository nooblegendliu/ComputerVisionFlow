add_library(VisionFlowNodes INTERFACE)
add_library(VisionFlow::Nodes ALIAS VisionFlowNodes)

target_sources(VisionFlowNodes INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/internal_nodes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Add/add.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Solid/Solid.hpp"
)

target_include_directories(VisionFlowNodes INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowNodes INTERFACE
    VisionFlow::Sdk
    nlohmann_json::nlohmann_json
)
