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
