cmake_minimum_required(VERSION 2.8.12...3.27)

project(mt32emu_infinstall CXX C)

# We resort to rely on a non-public API of libwdi to make use of the driver signing function.
# Sadly, the public API isn't very suitable for the purpose, so we require source distribution
# at a particular version.
set(libwdi_VERSION_REQUIRED 1.3.1)
find_path(libwdi_SRC_PATH configure.ac)
if(libwdi_SRC_PATH AND EXISTS ${libwdi_SRC_PATH}/configure.ac AND EXISTS ${libwdi_SRC_PATH}/libwdi/pki.c)
  message(STATUS "Found libwdi at ${libwdi_SRC_PATH}, checking version...")
  file(STRINGS ${libwdi_SRC_PATH}/configure.ac libwdi_SRC_ID LIMIT_COUNT 1)
  string(REGEX MATCH "^AC_INIT\\(\\[libwdi\\], \\[([.0-9]+)\\]" libwdi_SRC_ID ${libwdi_SRC_ID})
  set(libwdi_SRC_VERSION ${CMAKE_MATCH_1})
endif()
if(NOT libwdi_SRC_VERSION VERSION_EQUAL libwdi_VERSION_REQUIRED)
  message(FATAL_ERROR "Need libwdi ${libwdi_VERSION_REQUIRED} source package, download from
  https://github.com/pbatard/libwdi/releases/tag/v${libwdi_VERSION_REQUIRED}")
endif()
include_directories(${libwdi_SRC_PATH}/msvc)

set(${PROJECT_NAME}_SOURCES
  infinstall.cpp
  pki_helper.cpp
  ${libwdi_SRC_PATH}/libwdi/pki.c
)

if(MSVC_IDE)
  set_source_files_properties(infinstall.cpp pki_helper.cpp
    PROPERTIES COMPILE_FLAGS -Yu
  )
  set_source_files_properties(stdafx.cpp
    PROPERTIES COMPILE_FLAGS -Yc
  )
  list(APPEND ${PROJECT_NAME}_SOURCES stdafx.cpp)
endif()

if(CMAKE_CL_64)
  set(EXE_ARCH_SUFFIX _x64)
endif()

add_definitions(
  -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
  -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1
  -D_CRT_SECURE_NO_WARNINGS
)
add_executable(infinstall ${${PROJECT_NAME}_SOURCES})
target_link_libraries(infinstall newdev.lib SetupAPI.Lib)
set_target_properties(infinstall
  PROPERTIES
    LINK_FLAGS "/ENTRY:mainCRTStartup /MANIFESTUAC:level='requireAdministrator' /SUBSYSTEM:WINDOWS"
    OUTPUT_NAME infinstall${EXE_ARCH_SUFFIX}
)
