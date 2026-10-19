function(visionflow_register_plugin target)
    set_property(GLOBAL APPEND PROPERTY VISIONFLOW_PLUGIN_TARGETS ${target})
endfunction()

add_subdirectory(ImageLoader)
add_subdirectory(ImageWriter)

add_subdirectory(ShapeColorizer)
add_subdirectory(ShapeCounter)
add_subdirectory(SimpleBlobTracker)

add_subdirectory(VideoLoader)
add_subdirectory(VideoWriter)
add_subdirectory(VideoCapture)
add_subdirectory(DataOutput)



# ---------------------------------- Demo Add -----------------------------------
# Industrial localization demo
find_package(OpenCV CONFIG REQUIRED
    COMPONENTS core imgproc)

add_library(DemoWaferLocator SHARED
    IndustrialDemo/demo_wafer_locator.hpp
    IndustrialDemo/demo_wafer_locator.cpp
)

target_link_libraries(DemoWaferLocator PRIVATE
    VisionFlow::Sdk
    opencv_core
    opencv_imgproc
)

set_target_properties(DemoWaferLocator PROPERTIES
    PREFIX ""
    SUFFIX ".fp"
)

visionflow_register_plugin(DemoWaferLocator)

if(MSVC)
    target_compile_options(DemoWaferLocator PRIVATE /utf-8)
endif()
