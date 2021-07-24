# Find-module for GLib2, a general-purpose utility library. Can be found
# at https://download.gnome.org/ and https://gitlab.gnome.org/GNOME/glib.git/
# Defines variable GLIB2_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# GLib2::glib2 is created.

if(TARGET GLib2::glib2)
  if(NOT GLIB2_FIND_QUIETLY)
    message("Finding package GLIB2 skipped because target GLib2::glib2 already exists; assuming it is suitable.")
  endif()
  set(GLIB2_FOUND TRUE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_GLIB2 QUIET glib-2.0)
set(GLIB2_VERSION ${PC_GLIB2_VERSION})

find_path(GLIB2_INCLUDE_DIR glib.h
  HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
  PATH_SUFFIXES glib-2.0
)

find_path(GLIB2_CONFIG_INCLUDE_DIR glibconfig.h
  HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
  PATH_SUFFIXES lib/glib-2.0/include
)

find_library(GLIB2_LIBRARY glib-2.0
  HINTS ${PC_GLIB2_LIBDIR} ${PC_GLIB2_LIBRARY_DIRS}
)

mark_as_advanced(GLIB2_LIBRARY GLIB2_INCLUDE_DIR GLIB2_CONFIG_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set GLIB2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GLIB2
  REQUIRED_VARS GLIB2_LIBRARY GLIB2_INCLUDE_DIR GLIB2_CONFIG_INCLUDE_DIR
  VERSION_VAR GLIB2_VERSION
)

if(GLIB2_FOUND)
  add_library(GLib2::glib2 UNKNOWN IMPORTED)
  set_target_properties(GLib2::glib2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLIB2_INCLUDE_DIR};${GLIB2_CONFIG_INCLUDE_DIR}"
    IMPORTED_LOCATION "${GLIB2_LIBRARY}"
  )
endif()
