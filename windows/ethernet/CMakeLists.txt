cmake_minimum_required(VERSION 3.15)
project(omdrop_virtual_ethernet C)

include(CTest)

add_library(omdrop-virtual-ethernet STATIC virtual_ethernet.c)
target_include_directories(omdrop-virtual-ethernet PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(omdrop-virtual-ethernet PUBLIC c_std_99)

if(BUILD_TESTING)
  add_executable(virtual_ethernet_test virtual_ethernet_test.c)
  target_link_libraries(virtual_ethernet_test PRIVATE omdrop-virtual-ethernet)
  add_test(NAME virtual_ethernet_test COMMAND virtual_ethernet_test)
endif()
