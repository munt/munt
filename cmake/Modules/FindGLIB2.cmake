# - Try to find GLib2
# Once done this will define
#  GLIB2_FOUND - System has GLib2
#  GLIB2_INCLUDE_DIRS - The GLib2 include directory
#  GLIB2_LIBRARIES - Link these to use GLib2

include(FindPkgConfig)

find_package(PkgConfig)
pkg_search_module(PC_GLIB2 QUIET glib-2.0)

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

set(GLIB2_LIBRARIES ${GLIB2_LIBRARY})
set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIR} ${GLIB2_CONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set GLIB2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GLIB2 DEFAULT_MSG GLIB2_LIBRARY GLIB2_INCLUDE_DIR GLIB2_CONFIG_INCLUDE_DIR)

mark_as_advanced(GLIB2_LIBRARY GLIB2_INCLUDE_DIR GLIB2_CONFIG_INCLUDE_DIR)
