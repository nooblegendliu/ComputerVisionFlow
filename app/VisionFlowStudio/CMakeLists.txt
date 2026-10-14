# -------------------------------------------------------------------
# VisionFlowStudio
# -------------------------------------------------------------------

add_executable(VisionFlowStudio 
    main.cpp
    "MainWindow.h" 
    "MainWindow.cpp")

target_include_directories(VisionFlowStudio 
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(VisionFlowStudio 
    PRIVATE 
        VisionFlow::Editor 
        VisionFlow::Managers 
        VisionFlow::Utils 
        VisionFlow::Nodes
        Qt6::Widgets)


# ---------------------------------------------------------------------
#   Copy Runtime DLLs(Windows)
# ---------------------------------------------------------------------
if(WIN32)
    foreach(target
        VisionFlowStudio
    )
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endforeach()
endif()
