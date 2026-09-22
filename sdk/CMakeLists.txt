add_subdirectory(thirdParty)

add_library(VisionFlowSdk STATIC
    "flowTypes.h"
    "flowTypes.cpp"
    "flowProperties.h"
    "flowProperties.cpp"
)

add_library(VisionFlow::Sdk ALIAS VisionFlowSdk)

target_include_directories(VisionFlowSdk
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowSdk
    PUBLIC
        VisionFlow::DSPatch
        nlohmann_json::nlohmann_json
)
