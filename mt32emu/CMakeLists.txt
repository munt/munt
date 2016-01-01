cmake_minimum_required(VERSION 2.8.12)
project(libmt32emu CXX)
set(libmt32emu_VERSION_MAJOR 1)
set(libmt32emu_VERSION_MINOR 5)
set(libmt32emu_VERSION_PATCH 0)

add_definitions(-DMT32EMU_VERSION_MAJOR=${libmt32emu_VERSION_MAJOR})
add_definitions(-DMT32EMU_VERSION_MINOR=${libmt32emu_VERSION_MINOR})
add_definitions(-DMT32EMU_VERSION_PATCH=${libmt32emu_VERSION_PATCH})
add_definitions(-DMT32EMU_VERSION="${libmt32emu_VERSION_MAJOR}.${libmt32emu_VERSION_MINOR}.${libmt32emu_VERSION_PATCH}")

if(${munt_WITH_MT32EMU_SMF2WAV} OR ${munt_WITH_MT32EMU_QT})
  set(libmt32emu_STANDALONE_BUILD FALSE)
else()
  set(libmt32emu_STANDALONE_BUILD TRUE)
endif()

option(libmt32emu_SHARED "Build shared library" ${libmt32emu_STANDALONE_BUILD})

if(${libmt32emu_SHARED} AND NOT ${libmt32emu_STANDALONE_BUILD})
  if(${libmt32emu_C_INTERFACE} OR ${libmt32emu_PLUGIN_INTERFACE})
    message(STATUS "Options libmt32emu_C_INTERFACE and libmt32emu_PLUGIN_INTERFACE only supported for static library builds or stand-alone shared library builds, ignored")
    set(libmt32emu_C_INTERFACE FALSE)
    set(libmt32emu_PLUGIN_INTERFACE FALSE)
  endif(${libmt32emu_C_INTERFACE} OR ${libmt32emu_PLUGIN_INTERFACE})
else(${libmt32emu_SHARED} AND NOT ${libmt32emu_STANDALONE_BUILD})
  option(libmt32emu_C_INTERFACE "Provide C compatible external interface" ${libmt32emu_SHARED})
  option(libmt32emu_PLUGIN_INTERFACE "Provide plugin external interface via C++ abstract classes, implies libmt32emu_C_INTERFACE=FALSE" FALSE)
endif(${libmt32emu_SHARED} AND NOT ${libmt32emu_STANDALONE_BUILD})

set(libmt32emu_SOURCES
  src/Analog.cpp
  src/BReverbModel.cpp
  src/File.cpp
  src/FileStream.cpp
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
)

# Headers that always need to be installed:
set(libmt32emu_COMMON_HEADERS
  mt32emu.h
  globals.h
  Enumerations.h
)

# Headers used by C++ clients:
set(libmt32emu_CPP_HEADERS
  File.h
  FileStream.h
  MidiStreamParser.h
  ROMInfo.h
  Synth.h
  Types.h
)

# Headers that support C-compatible and plugin-style API:
set(libmt32emu_C_HEADERS
  c_interface/c_interface.h
  c_interface/c_types.h
  c_interface/cpp_interface.h
)

if(libmt32emu_C_INTERFACE OR libmt32emu_PLUGIN_INTERFACE)
  list(APPEND libmt32emu_SOURCES
    src/c_interface/c_interface.cpp
  )
  if(libmt32emu_SHARED)
    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  else(libmt32emu_SHARED)
    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_CPP_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  endif(libmt32emu_SHARED)
else(libmt32emu_C_INTERFACE OR libmt32emu_PLUGIN_INTERFACE)
  set(libmt32emu_HEADERS
    ${libmt32emu_COMMON_HEADERS}
    ${libmt32emu_CPP_HEADERS}
  )
endif(libmt32emu_C_INTERFACE OR libmt32emu_PLUGIN_INTERFACE)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -ansi -pedantic)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
endif()

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu)
foreach(HEADER ${libmt32emu_HEADERS})
  configure_file("src/${HEADER}" "include/mt32emu/${HEADER}" COPYONLY)
endforeach(HEADER)

if(libmt32emu_SHARED)
  if(libmt32emu_C_INTERFACE)
    add_definitions(-DMT32EMU_EXPORTS_TYPE=1)
  elseif(libmt32emu_PLUGIN_INTERFACE)
    add_definitions(-DMT32EMU_EXPORTS_TYPE=2)
  else(libmt32emu_C_INTERFACE)
    add_definitions(-DMT32EMU_EXPORTS_TYPE=0)
  endif(libmt32emu_C_INTERFACE)

  add_definitions(-DMT32EMU_SHARED)
  set(libmt32emu_BUILD_TYPE SHARED)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif(libmt32emu_SHARED)

add_library(mt32emu ${libmt32emu_BUILD_TYPE} ${libmt32emu_SOURCES})

install(TARGETS mt32emu
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu"
  DESTINATION include
)

install(FILES
  AUTHORS.txt COPYING.txt COPYING.LESSER.txt NEWS.txt README.txt TODO.txt
  DESTINATION share/doc/munt/libmt32emu
)

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These lines are irrelevant if compiling stand-alone.
if(NOT libmt32emu_STANDALONE_BUILD)
  set(MT32EMU_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE PATH "")
  set(MT32EMU_LIBRARY $<TARGET_LINKER_FILE:mt32emu> CACHE FILEPATH "")
endif()

# build a CPack driven installer package
set(CPACK_PACKAGE_VERSION_MAJOR "${libmt32emu_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${libmt32emu_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${libmt32emu_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "sergm@muntemu.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C/C++ library to emulate (approximately) the Roland MT-32, CM-32L and LAPC-I synthesiser modules")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "LGPLv2.1")
set(CPACK_RPM_PACKAGE_URL "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
