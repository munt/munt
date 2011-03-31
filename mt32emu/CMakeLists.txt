cmake_minimum_required(VERSION 2.6)
project(libmt32emu CXX)
set(libmt32emu_VERSION_MAJOR 0)
set(libmt32emu_VERSION_MINOR 1)
set(libmt32emu_VERSION_PATCH 3)

add_library(mt32emu STATIC
  src/ansiFile.cpp
  src/delayReverb.cpp
#  src/externalInterface.cpp
  src/file.cpp
  src/part.cpp
  src/partial.cpp
  src/partialManager.cpp
  src/poly.cpp
  src/synth.cpp
  src/tables.cpp
  src/tva.cpp
  src/tvf.cpp
  src/tvp.cpp
  src/freeverb/allpass.cpp
  src/freeverb/comb.cpp
  src/freeverb/revmodel.cpp
)
install(TARGETS mt32emu
  ARCHIVE DESTINATION lib
)
install(FILES
  src/file.h
  src/mt32emu.h
  src/part.h
  src/partial.h
  src/partialManager.h
  src/poly.h
  src/structures.h
  src/synth.h
  src/tables.h
  src/tva.h
  src/tvf.h
  src/tvp.h
  DESTINATION include/mt32emu
)
install(FILES
  src/freeverb/allpass.h
  src/freeverb/comb.h
  src/freeverb/denormals.h
  src/freeverb/tuning.h
  src/freeverb/revmodel.h
  DESTINATION include/mt32emu/freeverb
)

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
