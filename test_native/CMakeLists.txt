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
target_link_libraries(TestNative PRIVATE GTest::GTest GTest::Main PrometheusModule CURL::libcurl ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto Python3::Python)

# Temporary folder where tests are run
set(PYTHON_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/")
