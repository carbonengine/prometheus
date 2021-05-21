find_package(CURL CONFIG REQUIRED)
find_package(Python REQUIRED)
find_package(GTest CONFIG REQUIRED)

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
target_link_libraries(TestNative PRIVATE GTest::gmock_main PrometheusModule CURL::libcurl)

set(_PIP_DIR "${CMAKE_BINARY_DIR}/get-pip.py")
file(DOWNLOAD https://bootstrap.pypa.io/pip/2.7/get-pip.py ${_PIP_DIR})
add_custom_command(TARGET TestNative
    POST_BUILD
    COMMAND ${Python_EXECUTABLE} ${_PIP_DIR}
    COMMAND ${Python_EXECUTABLE} -m pip install --user pytest==4.6.11
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/tests"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PrometheusModule> $<TARGET_FILE:TestNative> ${CMAKE_SOURCE_DIR}/test/test.py "${CMAKE_BINARY_DIR}/tests"
    COMMAND ${Python_EXECUTABLE} -m pytest -v --junitxml=test_results.xml "${CMAKE_BINARY_DIR}/tests/test.py"
)
