# - Try to find libsoxr
# Once done this will define
#
# LIBSOXR_FOUND - system has libsoxr
# LIBSOXR_INCLUDE_DIRS - the libsoxr include directory
# LIBSOXR_LIBRARIES - Link these to use libsoxr
# LIBSOXR_DEFINITIONS - Compiler switches required for using libsoxr
#
# Adapted from cmake-modules Google Code project
#
# Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
# (Changes for libsoxr) Copyright (c) 2014 Matthias Kronlachner <m.kronlachner@gmail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBSOXR_LIBRARIES AND LIBSOXR_INCLUDE_DIRS)
  # in cache already
  set(LIBSOXR_FOUND TRUE)
else (LIBSOXR_LIBRARIES AND LIBSOXR_INCLUDE_DIRS)
  find_path(LIBSOXR_INCLUDE_DIR
    NAMES
      soxr.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(LIBSOXR_LIBRARY
    NAMES
      soxr
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(LIBSOXR_INCLUDE_DIRS
    ${LIBSOXR_INCLUDE_DIR}
  )
  set(LIBSOXR_LIBRARIES
    ${LIBSOXR_LIBRARY}
)

  if (LIBSOXR_INCLUDE_DIRS AND LIBSOXR_LIBRARIES)
     set(LIBSOXR_FOUND TRUE)
  endif (LIBSOXR_INCLUDE_DIRS AND LIBSOXR_LIBRARIES)

  if (LIBSOXR_FOUND)
    if (NOT libsoxr_FIND_QUIETLY)
      message(STATUS "Found libsoxr: ${LIBSOXR_LIBRARIES}")
    endif (NOT libsoxr_FIND_QUIETLY)
  else (LIBSOXR_FOUND)
    if (libsoxr_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libsoxr")
    endif (libsoxr_FIND_REQUIRED)
  endif (LIBSOXR_FOUND)

  # show the LIBSOXR_INCLUDE_DIRS and LIBSOXR_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBSOXR_INCLUDE_DIRS LIBSOXR_LIBRARIES)

endif (LIBSOXR_LIBRARIES AND LIBSOXR_INCLUDE_DIRS)