add_library(VisionFlowEditor STATIC
    VisionFlowEditor.h
    VisionFlowEditor.cpp
)

add_library(VisionFlow::Editor ALIAS VisionFlowEditor)

target_include_directories(VisionFlowEditor
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowEditor PRIVATE
    VisionFlow::Sdk
    VisionFlow::Managers
    QtNodes::QtNodes
)
