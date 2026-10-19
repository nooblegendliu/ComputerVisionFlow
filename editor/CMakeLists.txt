add_library(VisionFlowEditor STATIC
    NodeUi/NodeUiRegistry.h
    NodeUi/NodeUiRegistry.cpp
    VisionFlowEditor.h
    VisionFlowEditor.cpp

    Graph/VisionFlowGraphModel.hpp 
    Graph/VisionFlowGraphModel.cpp 
    
    PropertyPanel.h
    PropertyPanel.cpp 
)

add_library(
    VisionFlow::Editor 
    ALIAS VisionFlowEditor)

# VisionFlowGraphModel继承自QObject
# 使用 Q_Object必须启用moc
set_target_properties(VisionFlowEditor PROPERTIES 
    AUTOMOC ON
)

target_include_directories(VisionFlowEditor
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Graph"
)

target_link_libraries(VisionFlowEditor 
    PUBLIC 
        VisionFlow::Managers
        QtNodes::QtNodes
        Qt6::Widgets
        VisionFlow::QtNodeUi

    PRIVATE
        VisionFlow::Sdk
        Qt6::Core
)
