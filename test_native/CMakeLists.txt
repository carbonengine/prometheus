find_package(CURL CONFIG REQUIRED)
find_package(ZLIB CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(OpenSSL COMPONENTS Crypto SSL REQUIRED)
find_package(GTest REQUIRED)
find_package(Git REQUIRED)

add_library(TestNative SHARED
    module.cpp
    test.cpp
    test.h
)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(TestNative PROPERTIES
        OUTPUT_NAME test_native
        PREFIX ""
        SUFFIX ".so"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(TestNative PROPERTIES
        OUTPUT_NAME test_native
        PREFIX ""
        SUFFIX ".pyd"
    )
endif()
target_link_libraries(TestNative PRIVATE GTest::GTest GTest::Main PrometheusModule CURL::libcurl ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto)

# Temporary folder where tests are run
set(PYTHON_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/")

# Copy tests and pyds to the test folder
add_custom_command(TARGET TestNative POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${PYTHON_TEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../test/ ${PYTHON_TEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PrometheusModule> $<TARGET_FILE:TestNative> ${PYTHON_TEST_DIR}
    COMMAND ${Python3_EXECUTABLE} -m pytest -v -p no:cacheprovider --junitxml=${PYTHON_TEST_DIR}/test_results.xml "${PYTHON_TEST_DIR}/test.py"
)
