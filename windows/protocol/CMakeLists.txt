cmake_minimum_required(VERSION 3.15)
project(omdrop_radio_protocol C)

include(CTest)

add_library(omdrop-radio-protocol STATIC radio_protocol.c radio_messages.c)
target_include_directories(omdrop-radio-protocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(omdrop-radio-protocol PUBLIC c_std_99)

if(BUILD_TESTING)
  add_executable(radio_protocol_test radio_protocol_test.c)
  target_link_libraries(radio_protocol_test PRIVATE omdrop-radio-protocol)
  add_test(NAME radio_protocol_test COMMAND radio_protocol_test)
endif()
