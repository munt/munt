cmake_minimum_required(VERSION 2.8.12...3.27)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(mt32emu-smf2wav CXX)

include(cmake/project_data.cmake)

if(BUILD_SNAPSHOTS AND libmt32emu_SNAPSHOT_VERSION)
  include(../cmake/build_snapshots.cmake)
  add_definitions(-DBUILD_MT32EMU_VERSION="${libmt32emu_SNAPSHOT_VERSION}")
endif()

include(GNUInstallDirs)

add_definitions(-DMT32EMU_SMF2WAV_VERSION="${mt32emu_smf2wav_VERSION}")

find_package(GLIB2 2.32 MODULE REQUIRED)
list(APPEND EXT_LIBS GLib2::glib2)

if(NOT(munt_SOURCE_DIR AND TARGET MT32Emu::mt32emu))
  find_package(MT32Emu 2.7 CONFIG REQUIRED)
endif()
list(APPEND EXT_LIBS MT32Emu::mt32emu)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)

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

target_link_libraries(mt32emu-smf2wav PRIVATE
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
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
  AUTHORS.txt COPYING.txt README.md
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/munt/smf2wav
)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${mt32emu_smf2wav_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${mt32emu_smf2wav_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${mt32emu_smf2wav_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "${mt32emu_smf2wav_CONTACT}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/info.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${mt32emu_smf2wav_DESCRIPTION_SUMMARY}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)
# The default package file name has "-${CPACK_SYSTEM_NAME}" at the end, which doesn't match our previous releases
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${mt32emu_smf2wav_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RPM_PACKAGE_URL "${mt32emu_smf2wav_URL}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${mt32emu_smf2wav_URL}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
