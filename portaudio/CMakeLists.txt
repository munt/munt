# For a "How-To" please refer to the Portaudio documentation at:
# http://www.portaudio.com/trac/wiki/TutorialDir/Compile/CMake

# Note: This is modified for Munt, and differs significantly from
# the upstream version. In particular, the note on that web page
# that "Only Microsoft Visual C++ build files currently supported!"
# is invalid for this version.

set(CMAKE_LEGACY_CYGWIN_WIN32 0)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_support"
  "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/"
)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindPkgConfig)

project(libportaudio)

option(libportaudio_WITH_ALSA "Enable support for ALSA in PortAudio" YES)
option(libportaudio_WITH_JACK "Enable support for JACK in PortAudio" YES)
option(libportaudio_WITH_OSS "Enable support for OSS in PortAudio" YES)
option(libportaudio_WITH_ASIHPI "Enable support for ASIHPI in PortAudio" YES)
if(WIN32 AND MSVC)
  # Currently the following only work with MSVC for some reason
  option(libportaudio_WITH_ASIO "Enable support for ASIO in PortAudio" YES)
  option(libportaudio_WITH_DSOUND "Enable support for DirectSound in PortAudio" YES)
  if(libportaudio_WITH_DSOUND)
    option(libportaudio_USE_DIRECTSOUNDFULLDUPLEXCREATE "Use DirectSound full duplex create" YES)
    mark_as_advanced(libportaudio_USE_DIRECTSOUNDFULLDUPLEXCREATE)
  endif()
  option(libportaudio_WITH_WASAPI "Enable support for the Windows Audio Session API (WASAPI) in PortAudio" YES)
  option(libportaudio_WITH_WDMKS "Enable support for WDM Kernel Streaming in PortAudio" NO)
  option(libportaudio_USE_WDMKS_DEVICE_INFO "Use WDM/KS API for device info" NO)
  mark_as_advanced(libportaudio_USE_WDMKS_DEVICE_INFO)

  option(libportaudio_DLL_LINK_WITH_STATIC_RUNTIME "Link with static runtime libraries (minimises runtime dependencies)" YES)
endif()
if(WIN32 OR CYGWIN)
  option(libportaudio_WITH_WMME "Enable support for Windows Multimedia Extensions (MME) in PortAudio" YES)
  option(libportaudio_UNICODE_BUILD "Enable Portaudio Unicode build" NO)
endif()
option(libportaudio_ENABLE_DEBUG_OUTPUT "Enable debug output in PortAudio" NO)
option(libportaudio_BUILD_TESTS "Include test projects" NO)
mark_as_advanced(libportaudio_BUILD_TESTS)

set(libportaudio_COMMON_PUBLIC_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/portaudio.h"
)
set(libportaudio_UNIX_PUBLIC_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_jack.h"
)
set(libportaudio_LINUX_PUBLIC_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_linux_alsa.h"
)
set(libportaudio_DARWIN_PUBLIC_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_mac_core.h"
)
set(libportaudio_WINDOWS_PUBLIC_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_asio.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_win_ds.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_win_wasapi.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/pa_win_wmme.h"
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
source_group("common" FILES ${libportaudio_COMMON_SOURCES})

set(libportaudio_HOSTAPI_ALSA_SOURCES
  src/hostapi/alsa/pa_linux_alsa.c
)
set(libportaudio_HOSTAPI_ASIHPI_SOURCES
  src/hostapi/asihpi/pa_linux_asihpi.c
)
set(libportaudio_HOSTAPI_ASIO_SOURCES
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

set(libportaudio_OS_UNIX_SOURCES
  src/os/unix/pa_unix_hostapis.c
  src/os/unix/pa_unix_util.c
  src/os/unix/pa_unix_util.h
)
set(libportaudio_OS_WIN_SOURCES
  src/os/win/pa_win_coinitialize.c
  src/os/win/pa_win_hostapis.c
  src/os/win/pa_win_util.c
  src/os/win/pa_win_waveformat.c
)
set(libportaudio_OS_WIN_WDMKS_DEVICE_INFO_SOURCES
  src/os/win/pa_win_wdmks_utils.c
  src/os/win/pa_win_wdmks_utils.h
)

include_directories(include)
include_directories(src/common)

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
  check_include_files(sys/soundcard.h SYS_SOUNDCARD_FOUND)
  check_include_files(linux/soundcard.h LINUX_SOUNDCARD_FOUND)
  check_include_files(machine/soundcard.h MACHINE_SOUNDCARD_FOUND)
  if(SYS_SOUNDCARD_FOUND)
    add_definitions(-DHAVE_SYS_SOUNDCARD_H)
  endif()
  if(LINUX_SOUNDCARD_FOUND)
    add_definitions(-DHAVE_LINUX_SOUNDCARD_H)
  endif()
  if(MACHINE_SOUNDCARD_FOUND)
    add_definitions(-DHAVE_MACHINE_SOUNDCARD_H)
  endif()
  if(SYS_SOUNDCARD_FOUND OR LINUX_SOUNDCARD_FOUND OR MACHINE_SOUNDCARD_FOUND)
    check_library_exists(ossaudio _oss_ioctl "" LIBOSSAUDIO_FOUND)
    if(LIBOSSAUDIO_FOUND)
      list(APPEND libportaudio_EXT_LIBS ossaudio) # FIXME
    endif()
    add_definitions(-DPA_USE_OSS=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_OSS_SOURCES})
    list(APPEND libportaudio_PUBLIC_HEADERS
      ${libportaudio_UNIX_PUBLIC_HEADERS}
    )
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/unix)
    set(libportaudio_OS_SOURCES
      ${libportaudio_OS_UNIX_SOURCES}
    )
  endif()
endif()

if(libportaudio_WITH_ASIO)
  # Try to find ASIO SDK (assumes that portaudio and asiosdk folders are side-by-side, see
  # http://www.portaudio.com/trac/wiki/TutorialDir/Compile/WindowsASIOMSVC)
  find_package(ASIOSDK)
  if(ASIOSDK_FOUND)
    include_directories(${ASIOSDK_ROOT_DIR}/common)
    include_directories(${ASIOSDK_ROOT_DIR}/host)
    include_directories(${ASIOSDK_ROOT_DIR}/host/pc)
    set(libportaudio_ASIOSDK_SOURCES
      ${ASIOSDK_ROOT_DIR}/common/asio.cpp
      ${ASIOSDK_ROOT_DIR}/host/pc/asiolist.cpp
      ${ASIOSDK_ROOT_DIR}/host/asiodrivers.cpp
    )
    source_group("hostapi\\ASIO\\ASIOSDK" FILES
      ${libportaudio_ASIOSDK_SOURCES}
    )
    source_group("hostapi\\ASIO" FILES
      ${libportaudio_HOSTAPI_ASIO_SOURCES}
    )
    add_definitions(-DPA_USE_ASIO=1)
    list(APPEND libportaudio_HOSTAPI_SOURCES
      ${libportaudio_HOSTAPI_ASIO_SOURCES}
      ${libportaudio_ASIOSDK_SOURCES}
    )
  endif()
endif()

if(libportaudio_WITH_DSOUND)
  # Try to find DirectX SDK
  find_package(DXSDK)
  if(DXSDK_FOUND)
    # If we use DirectSound, we need this for the library to be found (if not in VS project settings)
    list(APPEND libportaudio_EXT_LIBS ${DXSDK_DSOUND_LIBRARY})
    include_directories(${DXSDK_INCLUDE_DIR})
    add_definitions(-DPA_USE_DS=1)
    if(libportaudio_USE_DIRECTSOUNDFULLDUPLEXCREATE)
      add_definitions(-DPAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE)
    endif()
    source_group("hostapi\\dsound" FILES ${libportaudio_HOSTAPI_DSOUND_SOURCES})
    list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_DSOUND_SOURCES})
  endif()
endif()

if(libportaudio_WITH_WASAPI)
  # FIXME: Actually check for WASAPI
#  if(MINGW)
#    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/hostapi/wasapi/mingw-include")
#  endif()
  add_definitions(-DPA_USE_WASAPI=1)
  source_group("hostapi\\wasapi" FILES ${libportaudio_HOSTAPI_WASAPI_SOURCES})
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WASAPI_SOURCES})
endif()

if(libportaudio_WITH_WDMKS)
  # If we use WDM/KS we need setupapi.lib
  list(APPEND libportaudio_EXT_LIBS setupapi)
  add_definitions(-DPA_USE_WDMKS=1)
  source_group("hostapi\\wdmks" FILES ${libportaudio_HOSTAPI_WDMKS_SOURCES})
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WDMKS_SOURCES})
endif()

if(libportaudio_WITH_WMME)
  # FIXME: Actually check for WMME
  add_definitions(-DPA_USE_WMME=1)
  source_group("hostapi\\wmme" FILES ${libportaudio_HOSTAPI_WMME_SOURCES})
  list(APPEND libportaudio_HOSTAPI_SOURCES ${libportaudio_HOSTAPI_WMME_SOURCES})
endif()

if(libportaudio_ENABLE_DEBUG_OUTPUT)
  add_definitions(-DPA_ENABLE_DEBUG_OUTPUT)
endif()

set(libportaudio_PUBLIC_HEADERS ${libportaudio_COMMON_PUBLIC_HEADERS})

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  list(APPEND libportaudio_PUBLIC_HEADERS
    ${libportaudio_UNIX_PUBLIC_HEADERS}
    ${libportaudio_LINUX_PUBLIC_HEADERS}
  )
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_UNIX_SOURCES}
  )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/unix)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  list(APPEND libportaudio_PUBLIC_HEADERS
    ${libportaudio_UNIX_PUBLIC_HEADERS}
    ${libportaudio_DARWIN_PUBLIC_HEADERS}
  )
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_UNIX_SOURCES}
  )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/unix/)

  list(APPEND libportaudio_EXT_LIBS "-framework CoreAudio" "-framework AudioToolbox" "-framework AudioUnit" "-framework Carbon")
  add_definitions(
    -DPA_USE_COREAUDIO
  )
  list(APPEND libportaudio_HOSTAPI_SOURCES
    ${libportaudio_HOSTAPI_COREAUDIO_SOURCES}
  )
elseif(WIN32 OR CYGWIN)
  list(APPEND libportaudio_PUBLIC_HEADERS
    ${libportaudio_WINDOWS_PUBLIC_HEADERS}
  )
  set(libportaudio_OS_SOURCES
    ${libportaudio_OS_WIN_SOURCES}
  )

  if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    if(libportaudio_DLL_LINK_WITH_STATIC_RUNTIME)
      foreach(flag_var
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
      )
        if(${flag_var} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
      endforeach()
    endif()
  endif()
  if(libportaudio_USE_WDMKS_DEVICE_INFO)
    add_definitions(-DPAWIN_USE_WDMKS_DEVICE_INFO)
    list(APPEND libportaudio_OS_WIN_SOURCES ${libportaudio_OS_WIN_WDMKS_DEVICE_INFO_SOURCES})
  endif()
  source_group("os\\win" FILES ${libportaudio_OS_WIN_SOURCES})
  list(APPEND libportaudio_EXT_LIBS winmm)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/os/win/)
endif()
source_group("include" FILES ${libportaudio_PUBLIC_HEADERS})

set(libportaudio_SOURCES
  ${libportaudio_HEADER}
  ${libportaudio_COMMON_SOURCES}
  ${libportaudio_OS_SOURCES}
  ${libportaudio_HOSTAPI_SOURCES}
)

if(libportaudio_UNICODE_BUILD)
  set(libportaudio_UNICODE_SOURCES ${libportaudio_SOURCES})

  # The "" here is a dummy, since REMOVE_ITEM requires at least two arguments and
  # libportaudio_ASIOSDK_SOURCES may be empty.
  list(REMOVE_ITEM libportaudio_UNICODE_SOURCES ${libportaudio_ASIOSDK_SOURCES} "")
  set_source_files_properties(
    ${libportaudio_UNICODE_SOURCES}
    PROPERTIES
    COMPILE_DEFINITIONS "UNICODE;_UNICODE"
  )
endif()

add_library(portaudio STATIC
  ${libportaudio_SOURCES}
)

target_link_libraries(portaudio
  ${libportaudio_EXT_LIBS}
)

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These lines are irrelevant if compiling stand-alone.
set(PORTAUDIO_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
get_target_property(PORTAUDIO_LIBRARY portaudio LOCATION)
set(PORTAUDIO_LIBRARIES "${PORTAUDIO_LIBRARY}" ${libportaudio_EXT_LIBS} PARENT_SCOPE)

# Prepared for inclusion of test files
if(libportaudio_BUILD_TESTS)
  subdirs(test)
endif()
