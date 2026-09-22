# Make imported targets available to sibling module directories.
find_package(spdlog CONFIG REQUIRED GLOBAL)
find_package(nlohmann_json CONFIG REQUIRED GLOBAL)

if(VISIONFLOW_BUILD_STUDIO)
    # Keep upstream's generic options local to this dependency.
    block(SCOPE_FOR VARIABLES)
        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        # Upstream first searches for the generic package name QT, whose
        # search does not use Qt6_ROOT. Reuse the Qt6 package found at the root.
        set(QT_DIR "${Qt6_DIR}")
        set(QT_NODES_DEVELOPER_DEFAULTS OFF)
        set(USE_QT6 ON)
        set(BUILD_SHARED_LIBS OFF)
        set(BUILD_EXAMPLES OFF)
        set(BUILD_TESTING OFF)
        set(BUILD_DOCS OFF)
        set(BUILD_DEBUG_POSTFIX_D OFF)
        add_subdirectory(nodeeditor)
    endblock()
endif()
