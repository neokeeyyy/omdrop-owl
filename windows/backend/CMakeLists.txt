cmake_minimum_required(VERSION 3.15)
project(omdrop_windows_backend C)

include(CTest)

add_library(omdrop-windows-backend-state STATIC backend_state.c)
target_include_directories(omdrop-windows-backend-state PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(omdrop-windows-backend-state PUBLIC c_std_99)

add_library(omdrop-windows-backend-radio STATIC backend_radio.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio/fake_radio.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_protocol.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_messages.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport/serial_transport.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../owl/radio_link.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../owl/radio_session.c"
)
target_include_directories(omdrop-windows-backend-radio PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio"
  "${CMAKE_CURRENT_SOURCE_DIR}/../owl"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport"
)
target_compile_features(omdrop-windows-backend-radio PUBLIC c_std_99)

add_executable(omdrop-windows-backend backend_cli.c)
target_link_libraries(omdrop-windows-backend PRIVATE
  omdrop-windows-backend-state
  omdrop-windows-backend-radio)

if(BUILD_TESTING)
  add_executable(backend_state_test backend_state_test.c)
  target_link_libraries(backend_state_test PRIVATE omdrop-windows-backend-state)
  add_test(NAME backend_state_test COMMAND backend_state_test)
  add_executable(backend_radio_test backend_radio_test.c)
  target_link_libraries(backend_radio_test PRIVATE omdrop-windows-backend-radio)
  add_test(NAME backend_radio_test COMMAND backend_radio_test)
  if(WIN32)
    add_test(NAME backend_cli_test
      COMMAND powershell -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/backend_cli_test.ps1")
  endif()
endif()
