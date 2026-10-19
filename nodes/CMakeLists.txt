find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc video features2d dnn)

# MSVC Debug uses a different CRT/STL ABI. Do not silently fall back to
# Release libraries when the local OpenCV installation has no Debug build.
if(MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES))
    
    get_target_property(_opencv_debug_library opencv_core IMPORTED_IMPLIB_DEBUG)

    get_target_property(_opencv_debug_location opencv_core IMPORTED_LOCATION_DEBUG)

    if(NOT _opencv_debug_library AND NOT _opencv_debug_location)
        message(FATAL_ERROR
            "This OpenCV installation has no MSVC Debug library. "
            "Install a matching Debug build and set OpenCV_DIR to it, "
            "or select a Release preset (local-x64-release on this machine).")
    endif()
endif()



# ------------------------------------------------------------------------
#   Internal Nodes Library
# ------------------------------------------------------------------------

add_library(VisionFlowNodes STATIC
    HistogramViewer/histogram_viewer.cpp
    DepthViewer3d/depth_viewer_3d.cpp
    DrawTools/draw_text.cpp
    DrawTools/draw_number.cpp
    DrawTools/draw_datetime.cpp
    DrawTools/draw_json.cpp
    BlobDetector/blob_detector.cpp
    HoughCircles/hough_circles.cpp
    HoughLines/hough_lines.cpp
    LineIntersect/line_intersect.cpp
    BackgroundSubtraction/background_subtraction.cpp
    GetOptimalDft/get_optimal_dft.cpp
    DiscreteFourierTransform/discrete_fourier_transform.cpp
    DiscreteCosineTransform/discrete_cosine_transform.cpp
    Magnitude/magnitude.cpp
    RndNoise/rnd_noise.cpp
    SobelFilter/sobel_filter.cpp
    ScharrFilter/scharr_filter.cpp
    LaplacianFilter/laplacian_filter.cpp
    Sharpen/sharpen.cpp
    Colorcorrect/ColorCorrect.cpp
    ColorReduce/color_reduce.cpp
    Split/split.cpp
    Combine/combine.cpp
    Crop/crop.cpp
    Transform/transform.cpp
    PerspectiveWarp/perspective_warp.cpp
    CopyMakeBorder/copy_make_border.cpp
    AbsDiff/abs_diff.cpp
    AddWeighted/add_weighted.cpp
    Subtract/subtract.cpp
    Multiply/multiply.cpp
    Divide/divide.cpp
    Max/max.cpp
    Min/min.cpp
    Bitwise/bitwise.cpp
    Mean/mean.cpp
    ScaleAbs/scale_abs.cpp
    Normalize/normalize.cpp
    ConvertColor/convert_color.cpp
    Threshold/threshold.cpp
    Morphology/Morphology.cpp
    Contours/contours.cpp
    JsonViewer/json_viewer.cpp
    GetSize/get_size.cpp

    # Node registration includes
    internal_nodes.h

    # Add
    Add/add.hpp
    Add/add.cpp

    # Solid
    Solid/solid.hpp
    Solid/solid.cpp

    # Viewer
    Viewer/viewer.hpp
    Viewer/viewer.cpp
    
    # Blur
    Blur/blur.hpp
    Blur/blur.cpp 
    
    # Resize
    Resize/resize.hpp
    Resize/resize.cpp 
    
    # Canny
    Canny/canny.hpp 
    Canny/canny.cpp)

add_library(
    VisionFlow::Nodes 
    ALIAS VisionFlowNodes)

# ------------------------------------------------------------------------
#   Include Directories
# ------------------------------------------------------------------------

target_include_directories(VisionFlowNodes 
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}"

    PRIVATE 
        ${OpenCV_INCLUDE_DIRS}
)

# ------------------------------------------------------------------------
#   Link Dependencies
# ------------------------------------------------------------------------

target_link_libraries(VisionFlowNodes 
    PUBLIC
        VisionFlow::Sdk
        nlohmann_json::nlohmann_json 
        opencv_core
        opencv_imgproc
        opencv_features2d
        opencv_video
)

add_subdirectory(DNN)
target_link_libraries(VisionFlowNodes PUBLIC opencv_dnn)
