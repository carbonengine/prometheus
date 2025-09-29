cmake_minimum_required(VERSION 3.16)

project(PrometheusModule)

include(cmake/CcpMinimumTargetPlatform.cmake)
include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpPacIdentifiers.cmake)
include(cmake/CcpGlobalSettings.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)

find_package(GTest REQUIRED)
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
    target_link_libraries(${target_name} PRIVATE Python3::Python)
endfunction()


# Actual projects
add_subdirectory(prometheus_module)
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR PROMETHEUS_MODULE_BUILD_TESTING) AND BUILD_TESTING)
    add_subdirectory(test_native)

    set(ENV{PYTHONPATH} ${CMAKE_CURRENT_SOURCE_DIR}/python)
    execute_process(COMMAND ${Python3_EXECUTABLE} discover.py
            OUTPUT_VARIABLE PYTHON_TESTS_STR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE PYTHON_TEST_DISCOVERY_RETVAL
            ERROR_VARIABLE PYTHON_TEST_DISCOVERY_ERRORS
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python
    )
    if(${PYTHON_TEST_DISCOVERY_RETVAL})
        message(FATAL_ERROR "Error encountered during test discovery:\n=== stderr: ===\n${PYTHON_TEST_DISCOVERY_ERRORS}\n=== stdout: ===\n${PYTHON_TESTS_STR}")
    endif()

    separate_arguments(PYTHON_TESTS UNIX_COMMAND ${PYTHON_TESTS_STR})
    foreach(PYTHON_TEST ${PYTHON_TESTS})
        message(STATUS "Found test ${PYTHON_TEST}")
        add_test(NAME ${PYTHON_TEST}
                COMMAND ${Python3_EXECUTABLE} -m unittest -v ${PYTHON_TEST}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/python
        )
        if(WIN32)
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT  "PYTHONPATH=$<TARGET_FILE_DIR:TestNative>\;$<TARGET_FILE_DIR:PrometheusModule>\;${CMAKE_CURRENT_SOURCE_DIR}/python\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python\;${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>;DLL_PATHS=$<TARGET_FILE_DIR:GTest::gtest>\;$<TARGET_FILE_DIR:GTest::gtest_main>")
        elseif (APPLE)
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/python:$<TARGET_FILE_DIR:TestNative>$<TARGET_FILE_DIR:PrometheusModule>::${CMAKE_CURRENT_SOURCE_DIR}/python:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/python3.12/lib-dynload:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python:${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
        else()
            message(FATAL_ERROR "Unsupported platform")
        endif ()
    endforeach()
endif()
