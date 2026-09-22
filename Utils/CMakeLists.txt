add_library(VisionFlowUtils STATIC)
add_library(VisionFlow::Utils ALIAS VisionFlowUtils)

target_include_directories(VisionFlowUtils
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(VisionFlowUtils PRIVATE /utf-8)
endif()

add_subdirectory(logging)
