# Copyright © 2021 CCP ehf.

add_python_extension(PrometheusModule SHARED
    counter.cpp
    counter.h
    counter_interface.h
    exposer.cpp
    exposer.h
    gauge.cpp
    gauge.h
    gauge_interface.h
    handler.cpp
    handler.h
    histogram.cpp
    histogram.h
    histogram_interface.h
    metric_factory.cpp
    metric_factory.h
    metric_registry.cpp
    metric_registry.h
    metric_registry_interface.h
    module.cpp
    summary.cpp
    summary.h
    summary_interface.h
    utilities.cpp
    utilities.h
)

FetchContent_GetProperties(prometheus-cpp SOURCE_DIR _PROMCPP_SOURCE_DIR BINARY_DIR _PROMCPP_BINARY_DIR)
set(_CIVETWEB_INCLUDE_DIR ${_PROMCPP_SOURCE_DIR}/3rdparty/civetweb/include)
if(NOT EXISTS ${_CIVETWEB_INCLUDE_DIR})
    message(FATAL_ERROR "Could not find civetweb includes at ${_CIVETWEB_INCLUDE_DIR}")
endif()

target_link_libraries(PrometheusModule PRIVATE prometheus-cpp::pull ZLIB::ZLIB)
# guaranteed to not reach this if zlib was not found by CMake.
# But for some reason not automatically added by the relevant CMake module, so we need to add the define ourselves.
target_compile_definitions(PrometheusModule PRIVATE HAVE_ZLIB _WINDLL)
target_include_directories(PrometheusModule PRIVATE
    ${_CIVETWEB_INCLUDE_DIR}
)
target_include_directories(PrometheusModule INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(PrometheusModule PROPERTIES
    OUTPUT_NAME prometheus_module
)
