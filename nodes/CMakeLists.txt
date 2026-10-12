find_package(OpenCV CONFIG REQUIRED COMPONENTS core)

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

    # Node registration includes
    "internal_nodes.h"

    # Add
    "Add/add.hpp"
    "Add/add.cpp"

    # Solid
    "Solid/Solid.hpp"
    "Solid/Solid.cpp"

    # Viewer
    "Viewer/Viewer.hpp"
    "Viewer/Viewer.cpp")

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
)
