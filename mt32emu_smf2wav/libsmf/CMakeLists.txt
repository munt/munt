cmake_minimum_required(VERSION 2.8.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

project(libsmf C)
set(libsmf_VERSION_MAJOR 1)
set(libsmf_VERSION_MINOR 3)

# We prefer to link embedded libsmf statically into higher-level projects to avoid version conflicts.
# This option enforces shared library build. Note that this version is modified, hence we don't want
# it to be installed system-wide. This script does NOT install the shared library, this should be
# handled some other way.
option(libsmf_SHARED "Build shared library" OFF)
mark_as_advanced(libsmf_SHARED)

if(libsmf_SHARED)
  if(MSVC)
    message(FATAL_ERROR "DLL build is unsupported with MSVC")
  endif()
  set(libsmf_BUILD_TYPE SHARED)
else()
  set(libsmf_BUILD_TYPE STATIC)
endif()

if(NOT MSVC)
  list(APPEND libsmf_EXT_LIBS m)
endif()

if(NOT(mt32emu-smf2wav_SOURCE_DIR AND TARGET GLib2::glib2))
  find_package(GLIB2 MODULE REQUIRED)
endif()

list(APPEND libsmf_EXT_LIBS GLib2::glib2)

add_definitions(-DG_LOG_DOMAIN="libsmf")

configure_file(
  src/config.h.in
  "${PROJECT_BINARY_DIR}/config.h"
)
include_directories("${PROJECT_BINARY_DIR}")

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_CRT_SECURE_NO_WARNINGS=1)
endif()

add_library(smf ${libsmf_BUILD_TYPE}
  src/smf.c
  src/smf_decode.c
  src/smf_load.c
  src/smf_save.c
  src/smf_tempo.c
)

target_link_libraries(smf PRIVATE ${libsmf_EXT_LIBS})
