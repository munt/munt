# Added for Munt.
# Note: This is a work in progress - barely tested, and only on Linux

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindPkgConfig)

project(libportaudio)

option(libportaudio_WITH_ALSA "Enable support for ALSA in PortAudio" YES)
option(libportaudio_WITH_JACK "Enable support for JACK in PortAudio" YES)
option(libportaudio_WITH_OSS "Enable support for OSS in PortAudio" YES)
option(libportaudio_WITH_ASIHPI "Enable support for ASIHPI in PortAudio" YES)
if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  option(libportaudio_WITH_ASIO "Enable support for ASIO in PortAudio" YES)
  option(libportaudio_WITH_DSOUND "Enable support for DirectSound in PortAudio" YES)
  option(libportaudio_WITH_WASAPI "Enable support for the Windows Audio Session API (WASAPI) in PortAudio" YES)
  option(libportaudio_WITH_WDMKS "Enable support for WDM Kernel Streaming in PortAudio" YES)
  option(libportaudio_WITH_WMME "Enable support for Windows Multimedia Extensions (MME) in PortAudio" YES)
endif()
option(libportaudio_ENABLE_DEBUG_OUTPUT "Enable debug output in PortAudio" NO)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(libportaudio_PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/portaudio.h
)

set(libportaudio_COMMON_SOURCES
  src/common/pa_allocation.c
  src/common/pa_allocation.h
  src/common/pa_converters.c
  src/common/pa_converters.h
  src/common/pa_cpuload.c
  src/common/pa_cpuload.h
  src/common/pa_debugprint.c
  src/common/pa_debugprint.h
  src/common/pa_dither.c
  src/common/pa_dither.h
  src/common/pa_front.c
  src/common/pa_process.c
  src/common/pa_process.h
  src/common/pa_ringbuffer.c # In the autotools build, this is only included when needed
  src/common/pa_ringbuffer.h
  src/common/pa_stream.c
  src/common/pa_stream.h
  src/common/pa_trace.c
  src/common/pa_trace.h
  src/hostapi/skeleton/pa_hostapi_skeleton.c

  src/common/pa_endianness.h
  src/common/pa_hostapi.h
  src/common/pa_memorybarrier.h
  src/common/pa_types.h
  src/common/pa_util.h
)

set(libportaudio_HOSTAPI_ALSA_SOURCES
  src/hostapi/alsa/pa_linux_alsa.c
)
set(libportaudio_HOSTAPI_ASIHPI_SOURCES
  src/hostapi/asihpi/pa_linux_asihpi.c
)
set(libportaudio_HOSTAPI_ASIO_SOURCES
  src/hostapi/asio/iasiothiscallresolver.cpp
  src/hostapi/asio/iasiothiscallresolver.h
  src/hostapi/asio/pa_asio.cpp
)
set(libportaudio_HOSTAPI_COREAUDIO_SOURCES
  src/hostapi/coreaudio/pa_mac_core_blocking.c
  src/hostapi/coreaudio/pa_mac_core_blocking.h
  src/hostapi/coreaudio/pa_mac_core.c
  src/hostapi/coreaudio/pa_mac_core_internal.h
  src/hostapi/coreaudio/pa_mac_core_utilities.c
  src/hostapi/coreaudio/pa_mac_core_utilities.h
)
set(libportaudio_HOSTAPI_DSOUND_SOURCES
  src/hostapi/dsound/pa_win_ds.c
  src/hostapi/dsound/pa_win_ds_dynlink.c
  src/hostapi/dsound/pa_win_ds_dynlink.h
)
set(libportaudio_HOSTAPI_JACK_SOURCES
  src/hostapi/jack/pa_jack.c
)
set(libportaudio_HOSTAPI_OSS_SOURCES
  src/hostapi/oss/pa_unix_oss.c
  src/hostapi/oss/recplay.c
)
set(libportaudio_HOSTAPI_WASAPI_SOURCES
  src/hostapi/wasapi/pa_win_wasapi.c
)
set(libportaudio_HOSTAPI_WDMKS_SOURCES
  src/hostapi/wdmks/pa_win_wdmks.c
)
set(libportaudio_HOSTAPI_WMME_SOURCES
  src/hostapi/wmme/pa_win_wmme.c
)

set(libportaudio_OS_WIN_SOURCES
  src/os/win/pa_win_hostapis.c
  src/os/win/pa_win_util.c
  src/os/win/pa_win_waveformat.c
  src/os/win/pa_win_wdmks_utils.c
  src/os/win/pa_win_wdmks_utils.h
)
set(libportaudio_OS_UNIX_SOURCES
  src/os/unix/pa_unix_hostapis.c
  src/os/unix/pa_unix_util.c
  src/os/unix/pa_unix_util.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/common)

if(libportaudio_WITH_ALSA)
  find_package(ALSA)
  if(ALSA_FOUND)
    list(APPEND libportaudio_EXT_LIBS ${ALSA_LIBRARIES})
    include_directories(${ALSA_INCLUDE_DIRS})
    add_definitions(-DPA_USE_ALSA=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_ALSA_SOURCES})
  endif()
endif()

if(libportaudio_WITH_ASIHPI)
  check_library_exists(hpi HPI_SubSysCreate "" ASIHPI_FOUND)
  if(ASIHPI_FOUND)
    add_definitions(-DPA_USE_ASIHPI=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_ASIHPI_SOURCES})
  endif()
endif()

if(libportaudio_WITH_JACK)
  find_package(JACK)
  if(JACK_FOUND)
    list(APPEND libportaudio_EXT_LIBS ${JACK_LIBRARIES})
    include_directories(${JACK_INCLUDE_DIRS})
    add_definitions(-DPA_USE_JACK=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_JACK_SOURCES})
  endif()
endif()

if(libportaudio_WITH_OSS)
  check_include_files(sys/soundcard.h linux/soundcard.h machine/soundcard.h OSS_FOUND)
  if(OSS_FOUND)
    check_library_exists(ossaudio _oss_ioctl "" LIBOSSAUDIO_FOUND)
    if(LIBOSSAUDIO_FOUND)
      list(APPEND libportaudio_EXT_LIBS ossaudio) # FIXME
    endif()
    add_definitions(-DPA_USE_OSS=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_OSS_SOURCES})
  endif()
endif()

if(libportaudio_WITH_ASIO)
  # FIXME: Actually check for ASIO
  add_definitions(-DPA_USE_ASIO=1)
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_ASIO_SOURCES})
endif()

if(libportaudio_WITH_DSOUND)
  # FIXME: Actually check for DirectSound
  add_definitions(-DPA_USE_DS=1)
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_DSOUND_SOURCES})
endif()

if(libportaudio_WITH_WASAPI)
  # FIXME: Actually check for WASAPI
  add_definitions(-DPA_USE_WASAPI=1)
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WASAPI_SOURCES})
endif()

if(libportaudio_WITH_WDMKS)
  # FIXME: Actually check for WDM/KS
  add_definitions(-DPA_USE_WDMKS=1)
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WDMKS_SOURCES})
endif()

if(libportaudio_WITH_WMME)
  # FIXME: Actually check for WMME
  add_definitions(-DPA_USE_WMME=1)
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WMME_SOURCES})
endif()

if(libportaudio_ENABLE_DEBUG_OUTPUT)
  add_definitions(-DPA_ENABLE_DEBUG_OUTPUT)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_UNIX_SOURCES}
  )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/unix)
  check_include_file("sys/soundcard.h" HAVE_SYS_SOUNDCARD_H)
  if (HAVE_SYS_SOUNDCARD_H)
    add_definitions(-DHAVE_SYS_SOUNDCARD_H)
  endif()
  check_include_file("linux/soundcard.h" HAVE_LINUX_SOUNDCARD_H)
  if (HAVE_LINUX_SOUNDCARD_H)
    add_definitions(-DHAVE_LINUX_SOUNDCARD_H)
  endif()
  check_include_file("machine/soundcard.h" HAVE_MACHINE_SOUNDCARD_H)
  if (HAVE_LINUX_SOUNDCARD_H)
    add_definitions(-DHAVE_MACHINE_SOUNDCARD_H)
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_UNIX_SOURCES}
  )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/unix/)

  add_definitions(
    -DPA_USE_COREAUDIO
  )
  set(libportaudio_HOSTAPI_SOURCES
    ${libportaudio_HOSTAPI_COREAUDIO_SOURCES}
  )
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_WIN_SOURCES}
  )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/win/)
  if (MINGW)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/hostapi/wasapi/mingw-include")
  endif()
endif()

set(libportaudio_SOURCES
  ${libportaudio_HEADER}
  ${libportaudio_COMMON_SOURCES}
  ${libportaudio_OS_SOURCES}
  ${libportaudio_HOSTAPI_SOURCES}
)

add_library(portaudio STATIC
  ${libportaudio_SOURCES}
)

target_link_libraries(portaudio
  ${libportaudio_EXT_LIBS}
)

install(
  TARGETS portaudio 
  ARCHIVE DESTINATION lib
)
install(
  FILES
  ${libportaudio_PUBLIC_HEADERS}
  DESTINATION include
)

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These lines are irrelevant if compiling stand-alone.
set(PORTAUDIO_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
get_target_property(PORTAUDIO_LIBRARY portaudio LOCATION)
set(PORTAUDIO_LIBRARIES "${PORTAUDIO_LIBRARY}" ${libportaudio_EXT_LIBS} PARENT_SCOPE)
