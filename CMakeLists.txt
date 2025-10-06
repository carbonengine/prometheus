cmake_minimum_required(VERSION 3.16)

project(PrometheusModule)

include(cmake/CcpMinimumTargetPlatform.cmake)
include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpPacIdentifiers.cmake)
include(cmake/CcpGlobalSettings.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpTargetConfigurations.cmake)

find_package(Python3 REQUIRED COMPONENTS Development Interpreter)

# Actual projects
add_subdirectory(prometheus_module)

option(BUILD_TESTING "Build tests. Enabled by default." ON)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)

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
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT  "PYTHONPATH=$<TARGET_FILE_DIR:TestNative>\;$<TARGET_FILE_DIR:PrometheusModule>\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python\;${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>;DLL_PATHS=$<TARGET_FILE_DIR:GTest::gtest>\;$<TARGET_FILE_DIR:GTest::gtest_main>")
        elseif (APPLE)
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:TestNative>:$<TARGET_FILE_DIR:PrometheusModule>:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python:${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
        else()
            message(FATAL_ERROR "Unsupported platform")
        endif ()
    endforeach()
endif()
