# Find-module for PulseAudio sound server system. Can be found at https://www.freedesktop.org/wiki/Software/PulseAudio/Download/
# and https://anongit.freedesktop.org/git/pulseaudio/pulseaudio.git/
# This module focuses on the simple PulseAudio API only that is used in mt32emu-qt. Defines variable
# PulseAudio_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# PulseAudio::simple is created.
#
# NOTE: Despite PulseAudio provides an official CMake config file, this module does not rely on it
# because the config file does not support the simple API (as of version 14). Instead, a pkg-config
# file is used to configure libpulse-simple, if found.

if(TARGET PulseAudio::simple)
  if(NOT PulseAudio_FIND_QUIETLY)
    message("Finding package PulseAudio skipped because target PulseAudio::simple already exists; assuming it is suitable.")
  endif()
  set(PulseAudio_FOUND TRUE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_PULSE_SIMPLE QUIET libpulse-simple)
set(PULSEAUDIO_VERSION ${PC_PULSE_SIMPLE_VERSION})
set(PULSEAUDIO_COMPILE_OPTIONS ${PC_PULSE_SIMPLE_CFLAGS_OTHER})

find_path(PULSEAUDIO_INCLUDE_DIR pulse/simple.h
  HINTS ${PC_PULSE_SIMPLE_INCLUDEDIR} ${PC_PULSE_SIMPLE_INCLUDE_DIRS}
)

find_library(PULSEAUDIO_LIBRARY pulse
  HINTS ${PC_PULSE_SIMPLE_LIBDIR} ${PC_PULSE_SIMPLE_LIBRARY_DIRS}
)

find_library(PULSEAUDIO_SIMPLE_LIBRARY pulse-simple
  HINTS ${PC_PULSE_SIMPLE_LIBDIR} ${PC_PULSE_SIMPLE_LIBRARY_DIRS}
)

mark_as_advanced(PULSEAUDIO_LIBRARY PULSEAUDIO_SIMPLE_LIBRARY PULSEAUDIO_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set PULSEAUDIO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PulseAudio
  REQUIRED_VARS PULSEAUDIO_SIMPLE_LIBRARY PULSEAUDIO_LIBRARY PULSEAUDIO_INCLUDE_DIR
  VERSION_VAR PULSEAUDIO_VERSION
)

if(CMAKE_VERSION VERSION_LESS 3.3)
  set(PulseAudio_FOUND ${PULSEAUDIO_FOUND})
endif()

if(PulseAudio_FOUND)
  add_library(PulseAudio::simple UNKNOWN IMPORTED)
  set_target_properties(PulseAudio::simple PROPERTIES
    INTERFACE_COMPILE_OPTIONS "${PULSEAUDIO_COMPILE_OPTIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${PULSEAUDIO_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${PULSEAUDIO_LIBRARY}"
    IMPORTED_LOCATION "${PULSEAUDIO_SIMPLE_LIBRARY}"
  )
endif()
