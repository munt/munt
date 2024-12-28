# Find-module for the doctest C++ testing framework - get from https://github.com/doctest/doctest/
# It's a header-only library, so the cmake package, that is normally deployed along, may be omitted.
# If the cmake package is found and sufficiently recent, it is used, and the INTERFACE target
# doctest::doctest is created as a result. Additionally, doctest_WITH_AUTO_TEST_DISCOVERY indicates
# whether a CMake test discovery script is available for doctest test cases.
#
# If a suitable cmake package is not found, or the used cmake does not support INTERFACE targets,
# this module searches for the doctest include directory. The following variables are defined:
# doctest_FOUND, doctest_INCLUDE_DIR, doctest_COMPILE_FEATURES, doctest_VERSION,
# doctest_VERSION_MAJOR, doctest_VERSION_MINOR, doctest_VERSION_PATCH.
# Note, the doctest source directory is also suitable as it contains the header file ready to use.

# Only try to find doctest cmake package if INTERFACE targets are supported.
if(NOT CMAKE_VERSION VERSION_LESS 3)
  # The doctest cmake package configured the include directories differently prior to v.2.2.2.
  # We rely on the new style.
  set(DOCTEST_MINIMUM_VERSION 2.2.2)
  if(DOCTEST_MINIMUM_VERSION VERSION_LESS doctest_FIND_VERSION)
    set(DOCTEST_MINIMUM_VERSION ${doctest_FIND_VERSION})
  endif()
  find_package(doctest ${DOCTEST_MINIMUM_VERSION} QUIET CONFIG)
  unset(DOCTEST_MINIMUM_VERSION)
  if(doctest_FOUND)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(doctest CONFIG_MODE)
    # The doctest cmake package lacked support for automatic tests discovery prior to v.2.3.3.
    # That is also unavailable if we have found the header file only.
    if(NOT doctest_VERSION VERSION_LESS 2.3.3)
      set(doctest_WITH_AUTO_TEST_DISCOVERY TRUE)
    endif()
    return()
  endif()
endif()

find_path(doctest_INCLUDE_DIR
  NAMES "doctest/doctest.h"
  DOC "The doctest include (source) directory"
)

mark_as_advanced(doctest_INCLUDE_DIR)

if(doctest_INCLUDE_DIR)
  file(STRINGS "${doctest_INCLUDE_DIR}/doctest/doctest.h" DOCTEST_VERSION_COMPONENTS
    LIMIT_COUNT 3
    REGEX ".* DOCTEST_VERSION_.*"
  )
  foreach(COMPONENT "MAJOR" "MINOR" "PATCH")
    if(DOCTEST_VERSION_COMPONENTS MATCHES " +DOCTEST_VERSION_${COMPONENT} +([0-9]+)")
      set(doctest_VERSION_${COMPONENT} ${CMAKE_MATCH_1})
    endif()
  endforeach()
  unset(DOCTEST_VERSION_COMPONENTS)
  set(doctest_VERSION "${doctest_VERSION_MAJOR}.${doctest_VERSION_MINOR}.${doctest_VERSION_PATCH}")
endif()

if(doctest_VERSION_MAJOR EQUAL 1)
  set(doctest_COMPILE_FEATURES "cxx_std_98")
else()
  set(doctest_COMPILE_FEATURES "cxx_std_11")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(doctest
  REQUIRED_VARS doctest_INCLUDE_DIR
  VERSION_VAR doctest_VERSION
)

if(CMAKE_VERSION VERSION_LESS 3.3)
  set(doctest_FOUND ${DOCTEST_FOUND})
endif()
