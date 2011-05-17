# - Try to find JACK
# Once done this will define
#  JACK_FOUND - System has JACK
#  JACK_INCLUDE_DIRS - The JACK include directory
#  JACK_LIBRARIES - Link these to use JACK

include(FindPkgConfig)

find_package(PkgConfig)
pkg_search_module(PC_JACK QUIET jack)

find_path(JACK_INCLUDE_DIR jack/jack.h
  HINTS ${PC_JACK_INCLUDEDIR} ${PC_JACK_INCLUDE_DIRS}
)

find_library(JACK_LIBRARY jack
  HINTS ${PC_JACK_LIBDIR} ${PC_JACK_LIBRARY_DIRS}
)

set(JACK_LIBRARIES ${JACK_LIBRARY})
set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set JACK_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(JACK DEFAULT_MSG JACK_LIBRARY JACK_INCLUDE_DIR)

mark_as_advanced(JACK_LIBRARY JACK_INCLUDE_DIR)
