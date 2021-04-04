# - Try to find mt32emu
# Once done this will define
#  MT32EMU_FOUND - System has mt32emu
#  MT32EMU_VERSION - The version of the mt32emu library
#  MT32EMU_INCLUDE_DIRS - The location of the mt32emu include directory
#  MT32EMU_LIBRARIES - The location of the mt32emu library

find_package(PkgConfig)
pkg_search_module(PC_MT32EMU QUIET mt32emu)

find_path(MT32EMU_INCLUDE_DIR mt32emu/mt32emu.h
  HINTS ${PC_MT32EMU_INCLUDEDIR} ${PC_MT32EMU_INCLUDE_DIRS}
)

find_library(MT32EMU_LIBRARY mt32emu
  HINTS ${PC_MT32EMU_LIBDIR} ${PC_MT32EMU_LIBRARY_DIRS}
)

file(STRINGS ${MT32EMU_INCLUDE_DIR}/mt32emu/config.h MT32EMU_VERSION_DEFINITIONS REGEX "define MT32EMU_VERSION")
foreach(DEFINITION ${MT32EMU_VERSION_DEFINITIONS})
  string(REGEX MATCH "(MT32EMU_VERSION[A-Z_]*) *\"?([0-9.]*)\"?" TAGGED_EXPORT ${DEFINITION})
  set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
endforeach()
unset(MT32EMU_VERSION_DEFINITIONS)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set MT32EMU_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(MT32EMU
  REQUIRED_VARS MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR
  VERSION_VAR MT32EMU_VERSION
)

# FIXME: Populate MT32EMU_EXT_LIBS in static builds properly. Maybe use a Cmake config file instead?
set(MT32EMU_LIBRARIES ${MT32EMU_LIBRARY} ${MT32EMU_EXT_LIBS})
set(MT32EMU_INCLUDE_DIRS ${MT32EMU_INCLUDE_DIR})

mark_as_advanced(MT32EMU_EXT_LIBS MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR)
