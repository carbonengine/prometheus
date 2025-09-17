add_library(CxxOpts INTERFACE IMPORTED)

set(CXXOPTS_DIR "${CMAKE_SOURCE_DIR}/vendor/jarro2783/cxxopts")

target_include_directories(CxxOpts INTERFACE ${CXXOPTS_DIR}/include)
