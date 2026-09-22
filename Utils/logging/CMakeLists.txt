target_sources(VisionFlowUtils PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp"
)

# The public logging header includes spdlog, so consumers need its usage requirements.
target_link_libraries(VisionFlowUtils
    PUBLIC spdlog::spdlog
)
