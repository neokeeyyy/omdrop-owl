cmake_minimum_required(VERSION 3.15)
project(omdrop_fake_radio C)

include(CTest)

add_library(omdrop-fake-radio STATIC fake_radio.c)
target_include_directories(omdrop-fake-radio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(omdrop-fake-radio PUBLIC
  omdrop-radio-transport
  omdrop-radio-protocol
)
target_compile_features(omdrop-fake-radio PUBLIC c_std_99)

if(BUILD_TESTING)
  add_executable(fake_radio_test fake_radio_test.c)
  target_link_libraries(fake_radio_test PRIVATE omdrop-fake-radio)
  add_test(NAME fake_radio_test COMMAND fake_radio_test)
endif()
