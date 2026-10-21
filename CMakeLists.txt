# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.25)


# ------------------------------------------------------------------------
#	MSVC Debug Information
# ------------------------------------------------------------------------

# Configure compiler policies before project() initializes the toolchain.
# Enable MSVC hot reload where supported, preserving the existing behavior.
if(POLICY CMP0141)

  cmake_policy(SET CMP0141 NEW)

  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT 
	  "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
  )
endif()

# ------------------------------------------------------------------------
#	Project
# ------------------------------------------------------------------------

project ("ComputerVisionFlow" 
		  LANGUAGES CXX 
		  VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------
#	Build Options
# ------------------------------------------------------------------------

option(VISIONFLOW_BUILD_STUDIO 
	   "Build the editor library and VisionFlowStudio application" ON
)

# ------------------------------------------------------------------------
#	QT Dependencies
# ------------------------------------------------------------------------

# Find Qt in the common parent scope of its consumers.
if(VISIONFLOW_BUILD_STUDIO)
  find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
endif()

# ------------------------------------------------------------------------
#	Core Dependencies
# ------------------------------------------------------------------------

# Third-party dependencies
add_subdirectory ("third_party")

# Utility library
add_subdirectory ("Utils")

# FlowCV SDK and DSPatch
add_subdirectory ("sdk")

# ------------------------------------------------------------------------
#	Runtime Modules
# ------------------------------------------------------------------------
# Nodes must be declared before Managers
add_subdirectory ("nodes")

# Manager depends on SDK，Nodes and Utils
add_subdirectory ("managers")

# Plugin implementations
add_subdirectory ("plugins")

# ------------------------------------------------------------------------
#	Editor / Studio
# ------------------------------------------------------------------------
if(VISIONFLOW_BUILD_STUDIO)
  add_subdirectory ("editor")
  add_subdirectory ("app")
endif()
