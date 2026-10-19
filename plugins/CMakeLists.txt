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
