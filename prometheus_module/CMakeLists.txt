find_package(prometheus-cpp CONFIG REQUIRED)
find_package(ZLIB CONFIG REQUIRED)

ccp_add_library(PrometheusModule SHARED
    counter.cpp
    counter.h
    exposer.cpp
    exposer.h
    gauge.cpp
    gauge.h
    handler.cpp
    handler.h
    histogram.cpp
    histogram.h
    metric_factory.cpp
    metric_factory.h
    metric_registry.cpp
    metric_registry.h
    module.cpp
    summary.cpp
    summary.h
    utilities.cpp
    utilities.h
)

target_link_libraries(PrometheusModule PRIVATE prometheus-cpp::pull ZLIB::ZLIB)
# guaranteed to not reach this if zlib was not found by CMake.
# But for some reason not automatically added by the relevant CMake module, so we need to add the define ourselves.
target_compile_definitions(PrometheusModule PRIVATE HAVE_ZLIB _WINDLL)
target_precompile_headers(PrometheusModule PRIVATE stdafx.h)
target_include_directories(PrometheusModule
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(PrometheusModule PROPERTIES
    OUTPUT_NAME prometheus_module
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(INSTALL_TO_MONOLITH "Install to perforce monolith" OFF)
    if (INSTALL_TO_MONOLITH)
        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
                TARGETS PrometheusModule
                EXPORT PrometheusTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
                LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
                RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}
        )

        # Generate target configuration for the monolith
        configure_ccp_vendor_config_file(
                TARGET PrometheusModule
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/EveMonolithPrometheusConfig.cmake"
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/EveMonolithPrometheusConfig.cmake DESTINATION .)
    else ()
        install(
                TARGETS PrometheusModule
                EXPORT PrometheusTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )

        install(EXPORT PrometheusTargets
                DESTINATION share/carbon-prometheus
                FILE carbon-prometheusConfig.cmake
        )

    endif ()

    install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h")
endif()
