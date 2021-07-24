# This module is DEPRECATED, do not use. Since version 2.6, mt32emu provides CMake config & version files.
# First, this module checks if target MT32Emu::mt32emu is already known in the build system. In this case,
# it is assumed that the existing target is a valid dependency and no further checks is performed.
# Otherwise, the module delegates finding mt32emu to find_package in explicitly set CONFIG mode.
# The configuration file creates the IMPORTED target MT32Emu::mt32emu, and for backwards compatibility,
# additionally sets the following variables:
# * MT32EMU_FOUND - if mt32emu is preset, this variable is TRUE;
# * MT32EMU_VERSION - the version of the found mt32emu library installation;
# * MT32EMU_INCLUDE_DIRS - the directory with the mt32emu C/C++ header files;
# * MT32EMU_LIBRARIES - contains the mt32emu library target for linking.
# If find_package fails in the CONFIG mode, an attempt is made to locate the mt32emu pkg-config file,
# and subsequently, to find the link library and the header files in common locations. Upon success,
# the same IMPORTED target MT32Emu::mt32emu is created and the above set of variables is defined.
#
# CAVEAT: If mt32emu is compiled as a static library with external dependencies, there is no easy and
# reliable way to configure the CMake build system correctly, except by directly using find_package
# in CONFIG mode and linking with the IMPORTED target MT32Emu::mt32emu.

if(NOT MT32EMU_FIND_VERSION VERSION_LESS 2.6)
  message(AUTHOR_WARNING "Usage of MT32EMU find-module is DEPRECATED, please use the provided CMake config file directly instead")
endif()

if(TARGET MT32Emu::mt32emu)
  if(NOT MT32EMU_FIND_QUIETLY)
    message("Finding package MT32EMU skipped because target MT32Emu::mt32emu already exists; assuming it is suitable.")
  endif()
  set(MT32EMU_FOUND TRUE)
  set(MT32EMU_LIBRARIES MT32Emu::mt32emu)
  set(MT32EMU_INCLUDE_DIRS "")
  return()
endif()

find_package(MT32Emu ${MT32EMU_FIND_VERSION} QUIET CONFIG)
if(MT32Emu_FOUND)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MT32Emu CONFIG_MODE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_MT32EMU QUIET mt32emu)

find_path(MT32EMU_INCLUDE_DIR mt32emu/mt32emu.h
  HINTS ${PC_MT32EMU_INCLUDEDIR} ${PC_MT32EMU_INCLUDE_DIRS}
)

find_library(MT32EMU_LIBRARY mt32emu
  HINTS ${PC_MT32EMU_LIBDIR} ${PC_MT32EMU_LIBRARY_DIRS}
)

unset(MT32EMU_VERSION)
if(MT32EMU_INCLUDE_DIR AND MT32EMU_LIBRARY)
  file(STRINGS ${MT32EMU_INCLUDE_DIR}/mt32emu/config.h MT32EMU_VERSION_DEFINITIONS REGEX "define MT32EMU_VERSION")
  foreach(MT32EMU_VERSION_DEFINITION ${MT32EMU_VERSION_DEFINITIONS})
    string(REGEX MATCH "(MT32EMU_VERSION[A-Z_]*) *\"?([0-9.]*)\"?" MT32EMU_VERSION_DEFINITION ${MT32EMU_VERSION_DEFINITION})
    set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
  endforeach()
  unset(MT32EMU_VERSION_DEFINITIONS)
endif()

mark_as_advanced(MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set MT32EMU_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(MT32EMU
  REQUIRED_VARS MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR
  VERSION_VAR MT32EMU_VERSION
)

if(MT32EMU_FOUND)
  add_library(MT32Emu::mt32emu UNKNOWN IMPORTED)
  set_target_properties(MT32Emu::mt32emu PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${MT32EMU_INCLUDE_DIR}"
    IMPORTED_LOCATION "${MT32EMU_LIBRARY}"
  )
  set(MT32EMU_LIBRARIES ${MT32EMU_LIBRARY})
  set(MT32EMU_INCLUDE_DIRS ${MT32EMU_INCLUDE_DIR})
endif()
