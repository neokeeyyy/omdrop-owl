cmake_minimum_required(VERSION 3.15)
project(omdrop_radio_transport C)

include(CTest)

if(NOT TARGET omdrop-radio-protocol)
  add_library(omdrop-radio-protocol STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_protocol.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_messages.c"
  )
  target_include_directories(omdrop-radio-protocol PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
  )
  target_compile_features(omdrop-radio-protocol PUBLIC c_std_99)
endif()

add_library(omdrop-radio-transport STATIC
  serial_transport.c
)
target_include_directories(omdrop-radio-transport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(omdrop-radio-transport PUBLIC omdrop-radio-protocol)
target_compile_features(omdrop-radio-transport PUBLIC c_std_99)

if(BUILD_TESTING)
  add_executable(serial_transport_test serial_transport_test.c)
  target_link_libraries(serial_transport_test PRIVATE
    omdrop-radio-transport
    omdrop-radio-protocol
  )
  add_test(NAME serial_transport_test COMMAND serial_transport_test)
endif()
