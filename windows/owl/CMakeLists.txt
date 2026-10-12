cmake_minimum_required(VERSION 3.15)
project(omdrop_owl_windows_core C)

include(CTest)

set(OWL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../upstream/owl" CACHE PATH
    "Checked-out OWL source directory")

if(NOT EXISTS "${OWL_SOURCE_DIR}/src/CMakeLists.txt")
  message(FATAL_ERROR
    "OWL source is missing. Run tools/windows/fetch-owl.ps1 first.")
endif()

add_library(owl-radiotap STATIC
  "${OWL_SOURCE_DIR}/radiotap/radiotap.c"
)
target_compile_definitions(owl-radiotap PRIVATE
  _BSD_SOURCE _DEFAULT_SOURCE RADIOTAP_SUPPORT_OVERRIDES
)
target_include_directories(owl-radiotap PUBLIC
  "${OWL_SOURCE_DIR}/radiotap"
  "${CMAKE_CURRENT_SOURCE_DIR}/compat"
)

set(OWL_CORE_SOURCES
  log.c state.c election.c sync.c channel.c schedule.c tx.c rx.c
  frame.c crc32.c wire.c peers.c version.c hashmap.c siphash24.c
  circular_buffer.c
)
add_library(owl-awdl-core STATIC)
foreach(source IN LISTS OWL_CORE_SOURCES)
  target_sources(owl-awdl-core PRIVATE "${OWL_SOURCE_DIR}/src/${source}")
endforeach()
target_include_directories(owl-awdl-core PUBLIC
  "${OWL_SOURCE_DIR}/src"
  "${OWL_SOURCE_DIR}/radiotap"
  "${CMAKE_CURRENT_SOURCE_DIR}/compat"
)
if(MINGW OR WIN32)
  target_compile_options(owl-awdl-core PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/compat/compat_win.h"
  )
endif()
target_link_libraries(owl-awdl-core PUBLIC owl-radiotap)
target_compile_features(owl-awdl-core PUBLIC c_std_99)

add_library(omdrop-radio-protocol STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_protocol.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/radio_messages.c"
)
target_include_directories(omdrop-radio-protocol PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
)
target_compile_features(omdrop-radio-protocol PUBLIC c_std_99)

add_library(omdrop-radio-transport STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport/serial_transport.c"
)
target_include_directories(omdrop-radio-transport PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport"
)
target_link_libraries(omdrop-radio-transport PUBLIC omdrop-radio-protocol)
target_compile_features(omdrop-radio-transport PUBLIC c_std_99)

add_library(omdrop-radio-link STATIC radio_link.c)
target_include_directories(omdrop-radio-link PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport"
)
target_link_libraries(omdrop-radio-link PUBLIC
  omdrop-radio-transport
)
target_compile_features(omdrop-radio-link PUBLIC c_std_99)

add_library(omdrop-radio-adapter STATIC radio_adapter.c)
target_include_directories(omdrop-radio-adapter PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${OWL_SOURCE_DIR}/src"
)
target_link_libraries(omdrop-radio-adapter PUBLIC
  omdrop-radio-link
  owl-awdl-core
)
target_compile_features(omdrop-radio-adapter PUBLIC c_std_99)

add_library(omdrop-radio-session STATIC radio_session.c)
target_include_directories(omdrop-radio-session PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../protocol"
  "${CMAKE_CURRENT_SOURCE_DIR}/../transport"
)
target_link_libraries(omdrop-radio-session PUBLIC omdrop-radio-link)
target_compile_features(omdrop-radio-session PUBLIC c_std_99)

add_library(omdrop-radio-loop STATIC radio_loop.c)
target_include_directories(omdrop-radio-loop PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(omdrop-radio-loop PUBLIC omdrop-radio-session)
target_compile_features(omdrop-radio-loop PUBLIC c_std_99)

add_library(omdrop-radio-runtime STATIC radio_runtime.c)
target_include_directories(omdrop-radio-runtime PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${OWL_SOURCE_DIR}/src"
)
target_link_libraries(omdrop-radio-runtime PUBLIC
  omdrop-radio-session
  omdrop-radio-adapter
)
target_compile_features(omdrop-radio-runtime PUBLIC c_std_99)

if(MSVC)
  target_compile_options(owl-awdl-core PRIVATE /W4)
else()
  target_compile_options(owl-awdl-core PRIVATE -Wall -Wextra)
endif()

if(BUILD_TESTING)
  add_executable(owl_core_smoke_test core_smoke_test.c)
  target_link_libraries(owl_core_smoke_test PRIVATE owl-awdl-core)
  target_include_directories(owl_core_smoke_test PRIVATE
    "${OWL_SOURCE_DIR}/src"
  )
  add_test(NAME owl_core_smoke_test COMMAND owl_core_smoke_test)

  add_executable(radio_link_test radio_link_test.c)
  target_link_libraries(radio_link_test PRIVATE omdrop-radio-link)
  add_test(NAME radio_link_test COMMAND radio_link_test)

  add_executable(radio_adapter_test radio_adapter_test.c)
  target_link_libraries(radio_adapter_test PRIVATE omdrop-radio-adapter)
  add_test(NAME radio_adapter_test COMMAND radio_adapter_test)

  add_executable(radio_session_test radio_session_test.c
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio/fake_radio.c")
  target_include_directories(radio_session_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio")
  target_link_libraries(radio_session_test PRIVATE omdrop-radio-session)
  add_test(NAME radio_session_test COMMAND radio_session_test)

  add_executable(radio_loop_test radio_loop_test.c
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio/fake_radio.c")
  target_include_directories(radio_loop_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio")
  target_link_libraries(radio_loop_test PRIVATE omdrop-radio-loop)
  add_test(NAME radio_loop_test COMMAND radio_loop_test)

  add_executable(radio_runtime_test radio_runtime_test.c
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio/fake_radio.c")
  target_include_directories(radio_runtime_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../fake-radio")
  target_link_libraries(radio_runtime_test PRIVATE omdrop-radio-runtime)
  add_test(NAME radio_runtime_test COMMAND radio_runtime_test)
endif()
