add_executable(theme_test ThemeTest.cpp)
set(_theme_studio_dir "${PROJECT_SOURCE_DIR}/app/VisionFlowStudio")
# Exercise the real Studio settings and menus, using its production sources.
get_target_property(_theme_studio_sources VisionFlowStudio SOURCES)
list(REMOVE_ITEM _theme_studio_sources main.cpp)
foreach(_source IN LISTS _theme_studio_sources)
    if(IS_ABSOLUTE "${_source}")
        target_sources(theme_test PRIVATE "${_source}")
    else()
        target_sources(theme_test PRIVATE "${_theme_studio_dir}/${_source}")
    endif()
endforeach()
set_target_properties(theme_test PROPERTIES AUTORCC ON)
target_include_directories(theme_test PRIVATE "${_theme_studio_dir}")
target_link_libraries(theme_test PRIVATE VisionFlow::Editor VisionFlow::Nodes VisionFlow::Utils Qt6::OpenGLWidgets)
add_dependencies(theme_test ImageLoader ImageWriter ShapeColorizer)
if(MSVC)
    target_compile_options(theme_test PRIVATE /utf-8)
endif()
if(WIN32)
    add_custom_command(TARGET theme_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:theme_test> "$<TARGET_FILE_DIR:theme_test>"
        COMMAND_EXPAND_LISTS VERBATIM)
endif()
add_test(NAME theme COMMAND theme_test "$<TARGET_FILE:ImageLoader>" "${PROJECT_BINARY_DIR}/theme-previews")
set_tests_properties(theme PROPERTIES TIMEOUT 60 ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>")
if(WIN32)
    # The offscreen plugin does not discover the Windows font directory itself.
    set_property(TEST theme APPEND PROPERTY ENVIRONMENT "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
endif()
