# A CMake find_package module to support using EveMonolithPrometheus from the Perforce vendor/ folder.
message(STATUS "Using vendored EveMonolithPrometheus package")

if(NOT TARGET EveMonolithPrometheus::EveMonolithPrometheus)
    add_library(EveMonolithPrometheus::EveMonolithPrometheus SHARED IMPORTED GLOBAL)

    set(EveMonolithPrometheus_FOUND TRUE)
    set(EveMonolithPrometheus_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
    set(EveMonolithPrometheus_LIBRARIES prometheus_module)

    set_target_properties(EveMonolithPrometheus::EveMonolithPrometheus PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${EveMonolithPrometheus_INCLUDE_DIRS}"
        OUTPUT_NAME prometheus_module
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(EveMonolithPrometheus_RUNTIME_LOCATION "${CMAKE_CURRENT_LIST_DIR}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/prometheus_module.pyd")
        set_target_properties(EveMonolithPrometheus::EveMonolithPrometheus PROPERTIES
            IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/${CCP_VENDOR_LIB_PATH}/prometheus_module.lib
            INTERFACE_LINK_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/${CCP_VENDOR_LIB_PATH}"
            IMPORTED_LOCATION ${EveMonolithPrometheus_RUNTIME_LOCATION}
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(EveMonolithPrometheus_RUNTIME_LOCATION "${CMAKE_CURRENT_LIST_DIR}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/prometheus_module.so")
        set_target_properties(EveMonolithPrometheus::EveMonolithPrometheus PROPERTIES
            IMPORTED_LOCATION ${EveMonolithPrometheus_RUNTIME_LOCATION}
        )
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()
endif()
