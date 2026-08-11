<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)

project(PrometheusModule)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)


find_package(Python3 REQUIRED COMPONENTS Development Interpreter)

# Actual projects
add_subdirectory(prometheus_module)

option(BUILD_TESTING "Build tests. Enabled by default." ON)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    find_package(GTest CONFIG REQUIRED)
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
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT  "PYTHONPATH=$<TARGET_FILE_DIR:TestNative>\;$<TARGET_FILE_DIR:PrometheusModule>\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python\;${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
        elseif (APPLE)
            set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:TestNative>:$<TARGET_FILE_DIR:PrometheusModule>:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python:${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
        else()
            message(FATAL_ERROR "Unsupported platform")
        endif ()
    endforeach()
endif()
=======
=======
>>>>>>> template/carbonengine/prometheus-updates
=======
>>>>>>> template/carbonengine/prometheus-updates
=======
>>>>>>> template/carbonengine/prometheus-updates
=======
>>>>>>> template/carbonengine/prometheus-updates
=======
>>>>>>> template/carbonengine/prometheus-updates
cmake_minimum_required(VERSION 3.16)
project(CarbonTemplateProject VERSION 1.0.0)

include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpTargetConfigurations.cmake)

ccp_add_library(target_name SHARED)

get_target_property(_SOURCES target_name SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

set_target_properties(target_name PROPERTIES OUTPUT_NAME "_target_name")

target_include_directories(target_name
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Provide an install target if this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Determine if/when to generate/build documentation. The rules are as follows:
    # - If running locally - Build documentation is OFF by default
    # - If running on a build agent (TeamCity) - Build documentation is ON by default
    # - If -DBUILD_DOCUMENTATION=ON/OFF flag is explicitly set, use it.
    set(BUILD_DOCUMENTATION_DEFAULT_FLAG OFF)
    if(DEFINED ENV{TEAMCITY_VERSION})
        set(BUILD_DOCUMENTATION_DEFAULT_FLAG ON)
    endif()
    message(STATUS "Document generation settings: default=${BUILD_DOCUMENTATION_DEFAULT_FLAG}, option BUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}")
    option(BUILD_DOCUMENTATION "Build Documentation" ${BUILD_DOCUMENTATION_DEFAULT_FLAG})

    if(BUILD_DOCUMENTATION)
        # Set up sphinx build for Python autodoc comments.
        set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/doc/source)
        set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/doc/build)

        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        include(cmake/CcpDocsGenerator.cmake)

        create_carbon_docs_sphinx_target(
                PYTHON_EXE ${Python3_EXECUTABLE}
                VENV_NAME docs_generation_venv
                PYTHONPATH_ENV $ENV{PYTHONPATH}
                SPHINX_SOURCE ${SPHINX_SOURCE}
                SPHINX_BUILD ${SPHINX_BUILD}
                DOXYGEN_SRC_FILES ${SRC_FILES}
                SPHINX_TARGET_NAME Sphinx
                DOXYGEN_TARGET_NAME Doxygen
                INSTALL_DESTINATION documentation
        )

        add_dependencies(Sphinx target_name)
    endif()


    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if (INSTALL_TO_MONOLITH)
        include(cmake/CcpMonolithVendorConfig.cmake)
        include(cmake/CcpPackageConfigHelpers.cmake)

        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
                TARGETS target_name
                EXPORT package_nameTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                LIBRARY DESTINATION ${CCP_VENDOR_LIB_PATH}
                ARCHIVE DESTINATION ${CCP_VENDOR_LIB_PATH}
                RUNTIME DESTINATION ${CCP_VENDOR_BIN_PATH}
        )

        # Generate target configuration for the monolith
        configure_ccp_vendor_config_file(
                TARGET target_name
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/package_nameConfig.cmake"
                DEPENDENCIES ""
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/package_nameConfig.cmake DESTINATION .)
        install(DIRECTORY python DESTINATION . FILES_MATCHING PATTERN "*.py")
    else ()
        # Install rule for available public headers
        install(
                TARGETS target_name
                EXPORT package_nameTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/package_name
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )

        install(EXPORT package_nameTarget
                DESTINATION share/package_name
                FILE package_name.cmake
        )

        install(
                FILES package_nameConfig.cmake
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/package_name)
    endif()

    install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h")

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
endif()
>>>>>>> template/carbonengine/prometheus-updates
=======
endif()
>>>>>>> template/carbonengine/prometheus-updates
=======
endif()
>>>>>>> template/carbonengine/prometheus-updates
=======
endif()
>>>>>>> template/carbonengine/prometheus-updates
=======
endif()
>>>>>>> template/carbonengine/prometheus-updates
=======
endif()
>>>>>>> template/carbonengine/prometheus-updates
