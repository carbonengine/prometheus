cmake_minimum_required(VERSION 3.16)

project(PrometheusModule)

include(cmake/CcpMinimumTargetPlatform.cmake)
include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpPacIdentifiers.cmake)
include(cmake/CcpGlobalSettings.cmake)


find_package(Python3 REQUIRED COMPONENTS Development Interpreter)

include(CTest)

# a convenience wrapper around add_library(target_name SHARED ...)
function(add_python_extension target_name)
    add_library(${target_name} SHARED ${ARGN})
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            SUFFIX ".so"
            )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_definitions(${target_name} PRIVATE _WINDLL)
        set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            SUFFIX ".pyd"
            )
    endif()
    target_link_libraries(${target_name} PUBLIC Python3::Python)
endfunction()


# Actual projects
add_subdirectory(prometheus_module)
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR PROMETHEUS_MODULE_BUILD_TESTING) AND BUILD_TESTING)
    add_subdirectory(test_native)
endif()
