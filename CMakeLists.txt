# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)

macro(require_path_in_environment_variable ENV_VAR_NAME OUTPUT_VAR)
    if(NOT DEFINED ENV{${ENV_VAR_NAME}})
        message(FATAL_ERROR "Missing required environment variable ${ENV_VAR_NAME}")
    elseif(NOT IS_ABSOLUTE $ENV{${ENV_VAR_NAME}})
        message(FATAL_ERROR "${ENV_VAR_NAME} needs to be an absolute path")
    elseif(NOT EXISTS $ENV{${ENV_VAR_NAME}})
        message(FATAL_ERROR "${ENV_VAR_NAME} is pointing at a non-existing location $ENV{${ENV_VAR_NAME}}")
    elseif(NOT IS_DIRECTORY $ENV{${ENV_VAR_NAME}})
        message(FATAL_ERROR "${ENV_VAR_NAME} needs to be a directory")
    else()
        message(STATUS "Using ${ENV_VAR_NAME} located at $ENV{${ENV_VAR_NAME}}")
    endif()

    file(TO_CMAKE_PATH "$ENV{${ENV_VAR_NAME}}" ${OUTPUT_VAR})
endmacro()
require_path_in_environment_variable(CCP_EVE_PERFORCE_BRANCH_PATH BRANCH_ROOT_DIR)

include(vendor/ccpgames/platform-cmake/CcpMinimumTargetPlatform.cmake)

# CCP vendors libraries using <packageName>Config.cmake files, so let's prefer that.
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

project(PrometheusModule)

include(vendor/ccpgames/platform-cmake/CcpBuildConfigurations.cmake)
include(vendor/ccpgames/platform-cmake/CcpPacIdentifiers.cmake)

set(GTest_ROOT "${BRANCH_ROOT_DIR}/vendor/GoogleTest/1.10.0")
set(Python_ROOT "${BRANCH_ROOT_DIR}/vendor/python/2.7.1+ccp-stackless")
set(CURL_ROOT "${BRANCH_ROOT_DIR}/vendor/libcurl/8.6.0")
set(ZLIB_ROOT "${BRANCH_ROOT_DIR}/vendor/zlib-ng/2.0.2")
set(OpenSSL_ROOT "${BRANCH_ROOT_DIR}/vendor/openssl/3.2.1")

find_package(GTest REQUIRED)
find_package(Git REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Python REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(ENABLE_TESTING OFF)  # Do not build the prometheus-cpp tests
FetchContent_Declare(
    prometheus-cpp
    GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(prometheus-cpp)

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
    target_link_libraries(${target_name} PUBLIC Python)
endfunction()

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(Python_EXECUTABLE "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.bat")
else()
    set(Python_EXECUTABLE "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.sh")
endif()

# Actual projects
add_subdirectory(prometheus_module)
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR PROMETHEUS_MODULE_BUILD_TESTING) AND BUILD_TESTING)
    add_subdirectory(test_native)
endif()
