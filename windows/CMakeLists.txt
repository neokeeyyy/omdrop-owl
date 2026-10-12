cmake_minimum_required(VERSION 3.15)
project(omdrop_windows C)

include(CTest)

add_subdirectory(protocol)
add_subdirectory(transport)
add_subdirectory(fake_radio)
add_subdirectory(ethernet)

option(OMDROP_WINDOWS_BUILD_OWL "Build the portable OWL integration" ON)
if(OMDROP_WINDOWS_BUILD_OWL)
  add_subdirectory(owl)
endif()

add_subdirectory(backend)
