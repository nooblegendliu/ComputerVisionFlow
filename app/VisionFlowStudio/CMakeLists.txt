add_executable(VisionFlowStudio main.cpp)

target_include_directories(VisionFlowStudio PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowStudio PRIVATE VisionFlow::Editor VisionFlow::Managers VisionFlow::Utils VisionFlow::Nodes)
