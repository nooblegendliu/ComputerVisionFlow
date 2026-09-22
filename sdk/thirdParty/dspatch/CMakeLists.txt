add_library(visionflow_dspatch STATIC
    include/DSPatch.h
    include/DSPatchVersion.h
    include/dspatch/Circuit.h
    include/dspatch/Common.h
    include/dspatch/Component.h
    include/dspatch/ComponentTypes.h
    include/dspatch/Plugin.h
    include/dspatch/Signal.h
    include/dspatch/SignalBus.h
    source/Circuit.cpp
    source/Component.cpp
    source/Plugin.cpp
    source/Signal.cpp
    source/SignalBus.cpp
    source/internal/AutoTickThread.cpp
    source/internal/AutoTickThread.h
    source/internal/CircuitThread.cpp
    source/internal/CircuitThread.h
    source/internal/ComponentThread.cpp
    source/internal/ComponentThread.h
    source/internal/Wire.h
)

add_library(VisionFlow::DSPatch ALIAS visionflow_dspatch)

target_include_directories(visionflow_dspatch SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(visionflow_dspatch PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)
target_compile_features(visionflow_dspatch PUBLIC cxx_std_20)
target_link_libraries(visionflow_dspatch PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
if(MSVC)
    target_compile_options(visionflow_dspatch PRIVATE /utf-8)
endif()
