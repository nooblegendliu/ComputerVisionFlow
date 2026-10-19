# -------------------------------------------------------------------
# VisionFlowStudio
# -------------------------------------------------------------------

add_executable(VisionFlowStudio 
    main.cpp

    MainWindow.h
    MainWindow.cpp
    
    ViewerPanel.h 
    ViewerPanel.cpp
    ViewerImage.cpp
    DrawTextControls.cpp
    DrawNumberControls.cpp
    DrawDateTimeControls.cpp
    DrawJsonControls.cpp
    HistogramPanel.cpp
    DepthViewerPanel.cpp
    DnnControls.cpp
    StudioNodeUis.cpp
    BlobControls.cpp
    HoughControls.cpp
    LineIntersectControls.cpp
    DerivativeControls.cpp
    CropControls.cpp
    PerspectiveControls.cpp
    BorderControls.cpp
    ColorCorrectControls.cpp
    SplitControls.cpp
    ConvertColorControls.cpp
    FilterControls.cpp
    AnalysisControls.cpp
    StudioSettings.cpp

    resources/studio.qrc
)

set_target_properties(VisionFlowStudio PROPERTIES
    AUTORCC ON
)
if(MSVC)
    target_compile_options(VisionFlowStudio PRIVATE /utf-8)
endif()

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
        Qt6::Widgets Qt6::OpenGLWidgets)


# Run on every Studio build, including when only a plugin was relinked.
add_custom_target(VisionFlowStudioPlugins
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VisionFlowStudio>/Plugins"
    VERBATIM
)
get_property(_studio_plugins GLOBAL PROPERTY VISIONFLOW_PLUGIN_TARGETS)
foreach(_plugin IN LISTS _studio_plugins)
    add_dependencies(VisionFlowStudioPlugins ${_plugin})
    add_custom_command(TARGET VisionFlowStudioPlugins POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${_plugin}>" "$<TARGET_FILE_DIR:VisionFlowStudio>/Plugins/"
        VERBATIM)
    if(WIN32)
    add_custom_command(TARGET VisionFlowStudioPlugins POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${_plugin}>
            "$<TARGET_FILE_DIR:VisionFlowStudio>"
        COMMAND_EXPAND_LISTS VERBATIM
    )
    endif()
endforeach()
add_dependencies(VisionFlowStudio VisionFlowStudioPlugins)

# ---------------------------------------------------------------------
#   Windows: Deploy runtime DLLs and Qt plugins
# ---------------------------------------------------------------------
if(WIN32)
    # locate the bin directory of the QT installation
    # actually used by CMake
    if(TARGET Qt6::qmake)
        get_target_property(
            _qt_qmake_executable
            Qt6::qmake
            IMPORTED_LOCATION
        )
        get_filename_component(
            _qt_bin_dir
            "${_qt_qmake_executable}"
            DIRECTORY
        )
    else()
        # Fallback for standard Qt installations.
        get_filename_component(
            _qt_prefix
            "${Qt6_DIR}/../../.."
            ABSOLUTE
        )

        set(_qt_bin_dir "${_qt_prefix}/bin")
    endif()

    find_program(
        VISIONFLOW_WINDEPLOYQT
        NAMES windeployqt.exe windeployqt
        HINTS "${_qt_bin_dir}"
        NO_DEFAULT_PATH
        REQUIRED
    )

    add_custom_command(
        TARGET VisionFlowStudio
        POST_BUILD

        # 1. Copy normal runtime DLL dependencies
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:VisionFlowStudio>
            $<TARGET_FILE_DIR:VisionFlowStudio>

        # 2. Deploy Qt libraries and plugins,
        #    including platforms/qwindows.dll
        COMMAND "${VISIONFLOW_WINDEPLOYQT}"
            --release
            --no-translations
            --no-compiler-runtime
            "$<TARGET_FILE:VisionFlowStudio>"

        COMMAND_EXPAND_LISTS
        VERBATIM

        COMMENT "Deploying VisionFlowStudio runtime dependencies"
    )
endif()
