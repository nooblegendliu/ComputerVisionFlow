add_executable(VisionFlowStudio main.cpp)

target_include_directories(VisionFlowStudio PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowStudio PRIVATE VisionFlow::Editor VisionFlow::Managers VisionFlow::Utils VisionFlow::Nodes)

if(WIN32)
    add_custom_command(TARGET VisionFlowStudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:VisionFlowStudio>
            $<TARGET_FILE_DIR:VisionFlowStudio>
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()
