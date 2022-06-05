cmake_minimum_required(VERSION 3.16)

add_library(basslib SHARED IMPORTED GLOBAL)

set_target_properties(basslib PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bass.lib"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bass.dll"
)

target_include_directories(basslib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)