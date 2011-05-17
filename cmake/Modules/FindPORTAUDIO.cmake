# - Try to find PortAudio
# Once done this will define
#  PORTAUDIO_FOUND - System has PortAudio
#  PORTAUDIO_INCLUDE_DIRS - The PortAudio include directory
#  PORTAUDIO_LIBRARIES - Link these to use PortAudio

if(DEFINED PORTAUDIO_INCLUDE_DIRS AND DEFINED PORTAUDIO_LIBRARIES)
  set(PORTAUDIO_FOUND TRUE)
  return()
endif()

include(FindPkgConfig)

find_package(PkgConfig)
pkg_search_module(PC_PORTAUDIO QUIET portaudio)

find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
  HINTS ${PC_PORTAUDIO_INCLUDEDIR} ${PC_PORTAUDIO_INCLUDE_DIRS}
)

find_library(PORTAUDIO_LIBRARY portaudio
  HINTS ${PC_PORTAUDIO_LIBDIR} ${PC_PORTAUDIO_LIBRARY_DIRS}
)

set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set PORTAUDIO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PORTAUDIO DEFAULT_MSG PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR)

mark_as_advanced(PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR)
