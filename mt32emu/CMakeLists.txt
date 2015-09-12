cmake_minimum_required(VERSION 2.8.12)
project(libmt32emu CXX)
set(libmt32emu_VERSION_MAJOR 1)
set(libmt32emu_VERSION_MINOR 5)
set(libmt32emu_VERSION_PATCH 0)

option(libmt32emu_SHARED "Build shared library" FALSE)

# Only headers that need to be installed should be listed here:
set(libmt32emu_HEADERS
  Enumerations.h
  globals.h
  File.h
  FileStream.h
  mt32emu.h
  MidiStreamParser.h
  ROMInfo.h
  Synth.h
  Types.h
  c_interface/c_interface.h
  c_interface/c_types.h
  c_interface/cpp_interface.h
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -ansi -pedantic)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
endif()

foreach(HEADER ${libmt32emu_HEADERS})
  configure_file("src/${HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu/${HEADER}" COPYONLY)
endforeach(HEADER)

if(libmt32emu_SHARED)
  set(libmt32emu_BUILD_TYPE SHARED)
  add_definitions(-DMT32EMU_SHARED)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

add_library(mt32emu ${libmt32emu_BUILD_TYPE}
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
  src/c_interface/c_interface.cpp
)

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
set(MT32EMU_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE PATH "")
set(MT32EMU_LIBRARY $<TARGET_LINKER_FILE:mt32emu> CACHE FILEPATH "")

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
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
