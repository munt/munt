# Find-module for JACK Audio Connection Kit. Can be found at https://jackaudio.org/downloads/
# and https://github.com/jackaudio/jack2/
# Defines variable JACK_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# Jack::jack is created.

if(TARGET Jack::jack)
  if(NOT JACK_FIND_QUIETLY)
    message("Finding package JACK skipped because target Jack::jack already exists; assuming it is suitable.")
  endif()
  set(JACK_FOUND TRUE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_JACK QUIET jack)
set(JACK_VERSION ${PC_JACK_VERSION})

find_path(JACK_INCLUDE_DIR jack/jack.h
  HINTS ${PC_JACK_INCLUDEDIR} ${PC_JACK_INCLUDE_DIRS}
)

find_library(JACK_LIBRARY
  NAMES jack libjack libjack64
  HINTS ${PC_JACK_LIBDIR} ${PC_JACK_LIBRARY_DIRS}
)

mark_as_advanced(JACK_LIBRARY JACK_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set JACK_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(JACK
  REQUIRED_VARS JACK_LIBRARY JACK_INCLUDE_DIR
  VERSION_VAR JACK_VERSION
)

if(JACK_FOUND)
  add_library(Jack::jack UNKNOWN IMPORTED)
  set_target_properties(Jack::jack PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${JACK_INCLUDE_DIR}"
    IMPORTED_LOCATION "${JACK_LIBRARY}"
  )
endif()
