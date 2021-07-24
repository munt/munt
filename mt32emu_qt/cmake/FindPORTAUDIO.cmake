# Find-module for PortAudio - portable audio I/O library. Can be found
# at http://files.portaudio.com/download.html and https://github.com/PortAudio/portaudio/
# Defines variable PORTAUDIO_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# portaudio is created.
#
# NOTE: This module is provided for compatibility with old versions found in some GNU distributions.
# Version 19.7 and above are expected to supply a CMake config file, which takes precedence if found.

if(TARGET portaudio)
  if(NOT PORTAUDIO_FIND_QUIETLY)
    message("Finding package PORTAUDIO skipped because target portaudio already exists; assuming it is suitable.")
  endif()
  set(PORTAUDIO_FOUND TRUE)
  return()
endif()

find_package(portaudio ${PORTAUDIO_FIND_VERSION} QUIET CONFIG)
if(portaudio_FOUND)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(portaudio CONFIG_MODE)
  set(PORTAUDIO_VERSION ${portaudio_VERSION})
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_PORTAUDIO QUIET portaudio-2.0)
set(PORTAUDIO_VERSION ${PC_PORTAUDIO_VERSION})

find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
  HINTS ${PC_PORTAUDIO_INCLUDEDIR} ${PC_PORTAUDIO_INCLUDE_DIRS}
)

find_library(PORTAUDIO_LIBRARY portaudio
  HINTS ${PC_PORTAUDIO_LIBDIR} ${PC_PORTAUDIO_LIBRARY_DIRS}
)

mark_as_advanced(PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set PORTAUDIO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PORTAUDIO
  REQUIRED_VARS PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR
  VERSION_VAR PORTAUDIO_VERSION
)

if(PORTAUDIO_FOUND)
  add_library(portaudio UNKNOWN IMPORTED)
  set_target_properties(portaudio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIR}"
    IMPORTED_LOCATION "${PORTAUDIO_LIBRARY}"
  )
endif()
