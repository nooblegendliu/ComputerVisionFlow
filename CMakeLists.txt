# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.25)

# Configure compiler policies before project() initializes the toolchain.
# Enable MSVC hot reload where supported, preserving the existing behavior.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ComputerVisionFlow" LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISIONFLOW_BUILD_STUDIO "Build the editor library and VisionFlowStudio application" ON)

# Find Qt in the common parent scope of its consumers.
if(VISIONFLOW_BUILD_STUDIO)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
endif()

# 公共模块与节点扩展。
add_subdirectory ("third_party")
add_subdirectory ("Utils")
add_subdirectory ("sdk")
add_subdirectory ("managers")
add_subdirectory ("nodes")
add_subdirectory ("plugins")

if(VISIONFLOW_BUILD_STUDIO)
  add_subdirectory ("editor")
  add_subdirectory ("app")
endif()
