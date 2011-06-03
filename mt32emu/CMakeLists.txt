cmake_minimum_required(VERSION 2.6)
project(libmt32emu CXX)
set(libmt32emu_VERSION_MAJOR 0)
set(libmt32emu_VERSION_MINOR 1)
set(libmt32emu_VERSION_PATCH 3)

# Only headers that need to be installed should be listed here:
set(libmt32emu_HEADERS
  src/File.h
  src/mt32emu.h
  src/LA32Ramp.h
  src/Part.h
  src/Partial.h
  src/Poly.h
  src/Structures.h
  src/Synth.h
  src/Tables.h
  src/TVA.h
  src/TVF.h
  src/TVP.h
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -ansi -pedantic)
endif()

foreach(HEADER ${libmt32emu_HEADERS})
  get_filename_component(FILENAME "${HEADER}" NAME)
  configure_file(${HEADER} "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu/${FILENAME}" COPYONLY)
endforeach(HEADER)

add_library(mt32emu STATIC
  src/ANSIFile.cpp
  src/AReverbModel.cpp
  src/DelayReverb.cpp
  src/File.cpp
  src/FreeverbModel.cpp
  src/LA32Ramp.cpp
  src/Part.cpp
  src/Partial.cpp
  src/PartialManager.cpp
  src/Poly.cpp
  src/Synth.cpp
  src/Tables.cpp
  src/TVA.cpp
  src/TVF.cpp
  src/TVP.cpp
  src/freeverb/allpass.cpp
  src/freeverb/comb.cpp
  src/freeverb/revmodel.cpp
)

install(TARGETS mt32emu
  ARCHIVE DESTINATION lib
)
install(DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu"
  DESTINATION include
)

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These lines are irrelevant if compiling stand-alone.
set(MT32EMU_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" PARENT_SCOPE)
get_target_property(MT32EMU_LIBRARY mt32emu LOCATION)
set(MT32EMU_LIBRARY "${MT32EMU_LIBRARY}" PARENT_SCOPE)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Static C++ library for embedding the Munt MT-32 emulator")
set(CPACK_PACKAGE_CONTACT "re_munt@kingguppy.com")
set(CPACK_PACKAGE_VERSION_MAJOR "${libmt32emu_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${libmt32emu_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${libmt32emu_VERSION_PATCH}")
include(CPack)
