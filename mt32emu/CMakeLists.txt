cmake_minimum_required(VERSION 2.8.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(libmt32emu CXX)
set(libmt32emu_DESCRIPTION "C/C++ library to emulate (approximately) the Roland MT-32, CM-32L and LAPC-I synthesiser modules")
set(libmt32emu_URL "http://munt.sourceforge.net/")
set(libmt32emu_VERSION_MAJOR 2)
set(libmt32emu_VERSION_MINOR 5)
set(libmt32emu_VERSION_PATCH 0)
set(libmt32emu_VERSION "${libmt32emu_VERSION_MAJOR}.${libmt32emu_VERSION_MINOR}.${libmt32emu_VERSION_PATCH}")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG TRUE)
else()
  set(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG FALSE)
endif()

# By default, we build a shared library, unless a higher-level script defines BUILD_SHARED_LIBS.
if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS TRUE)
endif()

option(libmt32emu_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ${libmt32emu_SHARED})

if(libmt32emu_REQUIRE_C_INTERFACE)
  unset(libmt32emu_C_INTERFACE CACHE)
  set(libmt32emu_C_INTERFACE TRUE)
else()
  option(libmt32emu_C_INTERFACE "Provide C-compatible API" TRUE)
endif()

if(libmt32emu_REQUIRE_CPP_INTERFACE OR NOT libmt32emu_SHARED)
  unset(libmt32emu_CPP_INTERFACE CACHE)
  set(libmt32emu_CPP_INTERFACE TRUE)
else()
  option(libmt32emu_CPP_INTERFACE "Provide C++ classes (compiler-specific ABI)" TRUE)
endif()

option(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER "Use built-in sample rate conversion" TRUE)

if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
  option(libmt32emu_REQUIRE_ANSI "Require ANSI C++ compatibility when compiling with GNU C++ or Clang" TRUE)
  mark_as_advanced(libmt32emu_REQUIRE_ANSI)
else()
  unset(libmt32emu_REQUIRE_ANSI CACHE)
endif()

set(LIB_INSTALL_DIR lib CACHE PATH "Relative installation path to libraries")

set(libmt32emu_PACKAGE_TYPE Complete CACHE STRING "Selects output binary package type: Complete, Runtime or Devel")
mark_as_advanced(libmt32emu_PACKAGE_TYPE)
set_property(CACHE libmt32emu_PACKAGE_TYPE PROPERTY STRINGS Complete Runtime Devel)

set(libmt32emu_PKGCONFIG_INSTALL_PREFIX ${LIB_INSTALL_DIR} CACHE PATH "Installation path prefix for pkgconfig files")
mark_as_advanced(libmt32emu_PKGCONFIG_INSTALL_PREFIX)

if(libmt32emu_SHARED)
  option(libmt32emu_WITH_VERSION_TAGGING "Tags shared object with library version and enables runtime version check")
  if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
    option(libmt32emu_WITH_SYMBOL_VERSIONING "Adds versions of version tags and C-compatible API symbols on supported platforms")
  else()
    unset(libmt32emu_WITH_SYMBOL_VERSIONING CACHE)
  endif()

  option(libmt32emu_INSTALL_DEFAULT_LOCALE "Install user-preferred system locale as C locale when initialising shared library.
 This is generally not necessary, although may be required in some cases (for instance, when building shared library with old
 versions of MS VC with the C runtime statically linked) to open ROM files with localised pathnames successfully.")
  mark_as_advanced(libmt32emu_INSTALL_DEFAULT_LOCALE)
else(libmt32emu_SHARED)
  unset(libmt32emu_WITH_VERSION_TAGGING CACHE)
  unset(libmt32emu_WITH_SYMBOL_VERSIONING CACHE)
  unset(libmt32emu_INSTALL_DEFAULT_LOCALE CACHE)
endif(libmt32emu_SHARED)

set(libmt32emu_SOURCES
  src/Analog.cpp
  src/BReverbModel.cpp
  src/File.cpp
  src/FileStream.cpp
  src/LA32FloatWaveGenerator.cpp
  src/LA32Ramp.cpp
  src/LA32WaveGenerator.cpp
  src/MidiStreamParser.cpp
  src/Part.cpp
  src/Partial.cpp
  src/PartialManager.cpp
  src/Poly.cpp
  src/ROMInfo.cpp
  src/Synth.cpp
  src/Tables.cpp
  src/TVA.cpp
  src/TVF.cpp
  src/TVP.cpp
  src/sha1/sha1.cpp
  src/SampleRateConverter.cpp
)

# Public headers that always need to be installed:
set(libmt32emu_COMMON_HEADERS
  mt32emu.h
  globals.h
  Enumerations.h
  Types.h
)

# Public headers used by C++ clients:
set(libmt32emu_CPP_HEADERS
  File.h
  FileStream.h
  MidiStreamParser.h
  ROMInfo.h
  SampleRateConverter.h
  Synth.h
)

# Public headers that support C-compatible and plugin-style API:
set(libmt32emu_C_HEADERS
  c_interface/c_interface.h
  c_interface/c_types.h
  c_interface/cpp_interface.h
)

if(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)
  # C++ API type
  set(libmt32emu_EXPORTS_TYPE 0)

  set(libmt32emu_HEADERS
    ${libmt32emu_COMMON_HEADERS}
    ${libmt32emu_CPP_HEADERS}
  )
else(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)
  list(APPEND libmt32emu_SOURCES
    src/c_interface/c_interface.cpp
  )
  # All API types
  if(libmt32emu_CPP_INTERFACE)
    set(libmt32emu_EXPORTS_TYPE 3)

    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_CPP_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  else(libmt32emu_CPP_INTERFACE)
    if(libmt32emu_C_INTERFACE)
      # C API type
      set(libmt32emu_EXPORTS_TYPE 1)
    else(libmt32emu_C_INTERFACE)
      # Plugin API type
      set(libmt32emu_EXPORTS_TYPE 2)
    endif(libmt32emu_C_INTERFACE)

    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  endif(libmt32emu_CPP_INTERFACE)
endif(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)

if(libmt32emu_WITH_VERSION_TAGGING)
  list(APPEND libmt32emu_SOURCES src/VersionTagging.cpp)
  list(APPEND libmt32emu_HEADERS VersionTagging.h)
  if(libmt32emu_WITH_SYMBOL_VERSIONING)
    set(libmt32emu_RUNTIME_VERSION_CHECK 1)
  else()
    set(libmt32emu_RUNTIME_VERSION_CHECK 2)
  endif()
else(libmt32emu_WITH_VERSION_TAGGING)
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu/VersionTagging.h)
  set(libmt32emu_RUNTIME_VERSION_CHECK 0)
endif(libmt32emu_WITH_VERSION_TAGGING)

if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
  add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -Wold-style-cast)
  if(libmt32emu_REQUIRE_ANSI)
    add_compile_options(-ansi -pedantic)
  endif()
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
endif()

if(libmt32emu_SHARED)
  set(libmt32emu_SHARED_DEFINITION "#define MT32EMU_SHARED")
  if(NOT OS2)
    # A workaround for the OS/2 port of GCC 9 that emits lots of warnings otherwise.
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  endif()
  if(libmt32emu_INSTALL_DEFAULT_LOCALE)
    add_definitions(-DMT32EMU_INSTALL_DEFAULT_LOCALE)
  endif()
else(libmt32emu_SHARED)
  set(libmt32emu_SHARED_DEFINITION "#undef MT32EMU_SHARED")
endif(libmt32emu_SHARED)

if(NOT libmt32emu_CURRENT_EXPORTS_TYPE STREQUAL libmt32emu_EXPORTS_TYPE)
  set(libmt32emu_CURRENT_EXPORTS_TYPE ${libmt32emu_EXPORTS_TYPE} CACHE INTERNAL "Stores current exports type")
  file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu)
endif()

foreach(HEADER ${libmt32emu_HEADERS})
  configure_file("src/${HEADER}" "include/mt32emu/${HEADER}" COPYONLY)
endforeach(HEADER)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu.h" "#include <mt32emu/mt32emu.h>")

configure_file("src/config.h.in" "include/mt32emu/config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu)

if(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)
  add_definitions(-DMT32EMU_WITH_INTERNAL_RESAMPLER)
  set(${PROJECT_NAME}_SOURCES ${${PROJECT_NAME}_SOURCES}
    src/srchelper/srctools/src/FIRResampler.cpp
    src/srchelper/srctools/src/SincResampler.cpp
    src/srchelper/srctools/src/IIR2xResampler.cpp
    src/srchelper/srctools/src/LinearResampler.cpp
    src/srchelper/srctools/src/ResamplerModel.cpp
    src/srchelper/InternalResampler.cpp
  )
else(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)
  # Prefer using SOXR if it is available
  find_package(LibSoxr)
  if(LIBSOXR_FOUND)
    set(libmt32emu_PC_REQUIRES_PRIVATE "soxr >= 0.1.2")
    add_definitions(-DMT32EMU_WITH_LIBSOXR_RESAMPLER)
    set(libmt32emu_EXT_LIBS ${libmt32emu_EXT_LIBS} ${LIBSOXR_LIBRARY})
    include_directories(${LIBSOXR_INCLUDE_DIR})
    set(${PROJECT_NAME}_SOURCES ${${PROJECT_NAME}_SOURCES}
      src/srchelper/SoxrAdapter.cpp
    )
  else(LIBSOXR_FOUND)
    message(STATUS "Could NOT find libsoxr")

    # Try Samplerate if SOXR is unavailable
    find_package(LibSamplerate)
    if(LIBSAMPLERATE_FOUND)
      set(libmt32emu_PC_REQUIRES_PRIVATE "samplerate >= 0.1.8")
      add_definitions(-DMT32EMU_WITH_LIBSAMPLERATE_RESAMPLER)
      set(libmt32emu_EXT_LIBS ${libmt32emu_EXT_LIBS} ${LIBSAMPLERATE_LIBRARY})
      include_directories(${LIBSAMPLERATE_INCLUDE_DIR})
      set(${PROJECT_NAME}_SOURCES ${${PROJECT_NAME}_SOURCES}
        src/srchelper/SamplerateAdapter.cpp
      )
    else(LIBSAMPLERATE_FOUND)
      message(STATUS "Could NOT find libsamplerate")
    endif(LIBSAMPLERATE_FOUND)
  endif(LIBSOXR_FOUND)
endif(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)

configure_file("src/mt32emu.pc.in" "mt32emu.pc" @ONLY)

add_library(mt32emu ${libmt32emu_SOURCES})

if(libmt32emu_EXT_LIBS)
  if(libmt32emu_SHARED)
    target_link_libraries(mt32emu
      ${libmt32emu_EXT_LIBS}
    )
    unset(MT32EMU_EXT_LIBS CACHE)
  else(libmt32emu_SHARED)
    # Locations are exported for the benefit of a higher-level CMakeLists.txt.
    # This is irrelevant if compiling stand-alone.
    set(MT32EMU_EXT_LIBS ${libmt32emu_EXT_LIBS} CACHE FILEPATH "")
  endif(libmt32emu_SHARED)
else(libmt32emu_EXT_LIBS)
  unset(MT32EMU_EXT_LIBS CACHE)
endif(libmt32emu_EXT_LIBS)

set_target_properties(mt32emu
  PROPERTIES VERSION ${libmt32emu_VERSION}
  SOVERSION ${libmt32emu_VERSION_MAJOR}
)

if(libmt32emu_SHARED AND CMAKE_SYSTEM_NAME STREQUAL Windows)
  set_target_properties(mt32emu
    PROPERTIES RUNTIME_OUTPUT_NAME mt32emu-${libmt32emu_VERSION_MAJOR}
  )
endif()

if(libmt32emu_WITH_SYMBOL_VERSIONING)
  set(${PROJECT_NAME}_VERSION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map)
  include(cmake/build_version_map.cmake)
  build_version_map("src/c_interface/c_interface.h" ${${PROJECT_NAME}_VERSION_SCRIPT})
  set_target_properties(mt32emu
    PROPERTIES LINK_FLAGS "-Wl,--version-script=${${PROJECT_NAME}_VERSION_SCRIPT}"
  )
endif()

if(libmt32emu_PACKAGE_TYPE STREQUAL "Runtime")
  if(libmt32emu_SHARED)
    install(TARGETS mt32emu
      LIBRARY DESTINATION ${LIB_INSTALL_DIR} NAMELINK_SKIP
      RUNTIME DESTINATION bin
    )
  endif()
elseif(libmt32emu_PACKAGE_TYPE STREQUAL "Devel")
  if(WIN32 OR CYGWIN)
    install(TARGETS mt32emu ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
  else()
    install(TARGETS mt32emu LIBRARY DESTINATION ${LIB_INSTALL_DIR} NAMELINK_ONLY)
  endif()
else()
  install(TARGETS mt32emu
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION bin
  )
endif()

if(NOT libmt32emu_PACKAGE_TYPE STREQUAL "Runtime")
  install(DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu"
    DESTINATION include
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu.h
    DESTINATION include
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mt32emu.pc
    DESTINATION ${libmt32emu_PKGCONFIG_INSTALL_PREFIX}/pkgconfig
  )
endif()

if(NOT(libmt32emu_SHARED AND libmt32emu_PACKAGE_TYPE STREQUAL "Devel"))
  install(FILES
    AUTHORS.txt COPYING.txt COPYING.LESSER.txt NEWS.txt README.md TODO.txt
    DESTINATION share/doc/munt/libmt32emu
  )
endif()

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These lines are irrelevant if compiling stand-alone.
set(MT32EMU_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE PATH "")
set(MT32EMU_LIBRARY $<TARGET_LINKER_FILE:mt32emu> CACHE FILEPATH "")

# build a CPack driven installer package
set(CPACK_PACKAGE_VERSION_MAJOR "${libmt32emu_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${libmt32emu_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${libmt32emu_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "sergm@muntemu.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../FreeBSD/usr/ports/audio/libmt32emu/pkg-descr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${libmt32emu_DESCRIPTION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
# The default package file name has "-${CPACK_SYSTEM_NAME}" at the end, which doesn't match our previous releases
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${libmt32emu_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "LGPLv2.1+")
set(CPACK_RPM_PACKAGE_URL ${libmt32emu_URL})
# Normally, pkgconfig files are installed into the system directory that should pre-exist on the target system,
# and hence cause conflict if not excluded
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/${libmt32emu_PKGCONFIG_INSTALL_PREFIX}/pkgconfig)
if(munt_SOURCE_DIR)
  # Also publish the list for the benefit of the parent CMakeLists.txt file.
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION} PARENT_SCOPE)
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${libmt32emu_URL})
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
if(libmt32emu_SHARED AND libmt32emu_PACKAGE_TYPE STREQUAL "Devel")
  set(CPACK_RPM_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-devel")
  set(CPACK_RPM_FILE_NAME "${CPACK_RPM_PACKAGE_NAME}-${libmt32emu_VERSION}.rpm")
  set(CPACK_RPM_PACKAGE_REQUIRES "${CMAKE_PROJECT_NAME} == ${libmt32emu_VERSION}")
  set(CPACK_DEBIAN_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-dev")
  set(CPACK_DEBIAN_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-${libmt32emu_VERSION}.deb")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CMAKE_PROJECT_NAME} (= ${libmt32emu_VERSION})")
endif()
if(libmt32emu_SHARED AND NOT libmt32emu_PACKAGE_TYPE STREQUAL "Devel")
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS "ON")
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
endif()

include(CPack)
