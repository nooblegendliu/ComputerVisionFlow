find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgcodecs)

# Keep FlowCV's DSPatch Create entry point and .fp shared-library format.
add_library(ImageLoader SHARED image_loader.hpp image_loader.cpp)
target_link_libraries(ImageLoader PRIVATE VisionFlow::Sdk opencv_core opencv_imgcodecs)
set_target_properties(ImageLoader PROPERTIES PREFIX "" SUFFIX ".fp")
visionflow_register_plugin(ImageLoader)
if(MSVC)
    target_compile_options(ImageLoader PRIVATE /utf-8)
endif()
