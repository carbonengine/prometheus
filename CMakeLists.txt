cmake_minimum_required(VERSION 3.16)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_SYSTEM_VERSION "10.0.17763")
endif()

project(PrometheusModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/vendor/ccpgames/platform-cmake")

option(ENABLE_TESTING "Build tests. Enabled by default." ON)

# Actual projects
add_subdirectory(prometheus_module)
if(ENABLE_TESTING)
    add_subdirectory(test_native)
endif()