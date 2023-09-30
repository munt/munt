cmake_minimum_required(VERSION 2.8.12...3.27)
project(munt)

include(cmake/project_data.cmake)

option(munt_WITH_MT32EMU_SMF2WAV "Build command line standard MIDI file conversion tool" TRUE)
option(munt_WITH_MT32EMU_QT "Build Qt-based UI-enabled application" TRUE)

if(MSVC)
  option(munt_WITH_MT32EMU_WIN32DRV "Build Windows MME MIDI driver" TRUE)
else()
  unset(munt_WITH_MT32EMU_WIN32DRV CACHE)
endif()

# By default, mt32emu is built stand-alone as a shared library, but as a static library otherwise.
if(NOT DEFINED BUILD_SHARED_LIBS)
  if(munt_WITH_MT32EMU_SMF2WAV OR munt_WITH_MT32EMU_QT OR munt_WITH_MT32EMU_WIN32DRV)
    set(BUILD_SHARED_LIBS FALSE)
  else()
    set(BUILD_SHARED_LIBS TRUE)
  endif()
endif()

if(munt_WITH_MT32EMU_SMF2WAV)
  set(libmt32emu_REQUIRE_C_INTERFACE TRUE)
endif()
if(munt_WITH_MT32EMU_QT OR munt_WITH_MT32EMU_WIN32DRV)
  set(libmt32emu_REQUIRE_CPP_INTERFACE TRUE)
endif()

add_subdirectory(mt32emu)

if(munt_WITH_MT32EMU_SMF2WAV)
  add_subdirectory(mt32emu_smf2wav)
  add_dependencies(mt32emu-smf2wav mt32emu)
endif()

if(munt_WITH_MT32EMU_QT)
  add_subdirectory(mt32emu_qt)
  add_dependencies(mt32emu-qt mt32emu)
endif()

if(munt_WITH_MT32EMU_WIN32DRV)
  add_subdirectory(mt32emu_win32drv)
  add_dependencies(win32drv mt32emu)
endif()

# build a CPack driven installer package
set(CPACK_PACKAGE_VERSION_MAJOR "${munt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${munt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${munt_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "${munt_CONTACT}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${munt_DESCRIPTION_SUMMARY}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)
# The default package file name has "-${CPACK_SYSTEM_NAME}" at the end, which doesn't match our previous releases
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${munt_VERSION}")

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)

if(NOT libmt32emu_PACKAGE_TYPE STREQUAL "Complete")
  set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR}" "${PROJECT_NAME}" "${libmt32emu_PACKAGE_TYPE}" "/")
endif()

set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "LGPLv2.1+, GPLv3+")
set(CPACK_RPM_PACKAGE_URL "${munt_URL}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${munt_URL}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
