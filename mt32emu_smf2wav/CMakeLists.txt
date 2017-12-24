cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-smf2wav CXX)
set(mt32emu_smf2wav_VERSION_MAJOR 1)
set(mt32emu_smf2wav_VERSION_MINOR 5)
set(mt32emu_smf2wav_VERSION_PATCH 0)
set(mt32emu_smf2wav_VERSION "${mt32emu_smf2wav_VERSION_MAJOR}.${mt32emu_smf2wav_VERSION_MINOR}.${mt32emu_smf2wav_VERSION_PATCH}")

add_definitions(-DVERSION="${mt32emu_smf2wav_VERSION}")

if(libmt32emu_SHARED)
  add_definitions(-DMT32EMU_SHARED)
endif()

find_package(GLIB2 REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${GLIB2_LIBRARIES})
include_directories(${GLIB2_INCLUDE_DIRS})

find_package(MT32EMU REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${MT32EMU_LIBRARIES})
include_directories(${MT32EMU_INCLUDE_DIRS})

include_directories(libsmf/src)
add_subdirectory(libsmf)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -ansi -pedantic)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
endif()

add_executable(mt32emu-smf2wav
  src/mt32emu-smf2wav.cpp
)

target_link_libraries(mt32emu-smf2wav
  smf
  ${EXT_LIBS}
)

if(WIN32)
  set_target_properties(mt32emu-smf2wav
    PROPERTIES VERSION ${mt32emu_smf2wav_VERSION}
  )
endif()

install(TARGETS
  mt32emu-smf2wav
  DESTINATION bin
)

install(FILES
  AUTHORS.txt COPYING.txt README.txt
  DESTINATION share/doc/munt/smf2wav
)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${mt32emu_smf2wav_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${mt32emu_smf2wav_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${mt32emu_smf2wav_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "sergm@muntemu.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../FreeBSD/usr/ports/audio/mt32emu_smf2wav/pkg-descr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Experimental program for converting SMF (Standard MIDI File) to WAVE via libmt32emu")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_URL "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
