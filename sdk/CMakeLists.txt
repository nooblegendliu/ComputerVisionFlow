add_subdirectory(thirdParty)

add_library(VisionFlowSdk STATIC
    "flowTypes.h"
    "flowTypes.cpp"
    "flowProperties.h"
    "flowProperties.cpp"
    "flowNode.h" 
    "inspectionResult.h" 
    "inspectionResult.cpp")

add_library(VisionFlow::Sdk ALIAS VisionFlowSdk)

target_include_directories(VisionFlowSdk
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowSdk
    PUBLIC
        VisionFlow::DSPatch
        nlohmann_json::nlohmann_json
)

# Optional Qt contract is available before plugin targets are declared.
if(VISIONFLOW_BUILD_STUDIO)
    add_library(VisionFlowQtNodeUi INTERFACE)
    add_library(VisionFlow::QtNodeUi ALIAS VisionFlowQtNodeUi)
    target_include_directories(VisionFlowQtNodeUi INTERFACE "${PROJECT_SOURCE_DIR}/editor")
    target_link_libraries(VisionFlowQtNodeUi INTERFACE VisionFlow::Sdk Qt6::Widgets)
endif()
