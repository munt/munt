cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-qt)
set(mt32emu_qt_VERSION_MAJOR 0)
set(mt32emu_qt_VERSION_MINOR 1)
set(mt32emu_qt_VERSION_PATCH 0)

set(mt32emu_qt_SOURCES
  src/ALSADriver.cpp
#  src/Win32Driver.cpp

  src/MidiEvent.cpp
  src/MidiEventQueue.cpp
  src/QSynth.cpp
  src/main.cpp
  src/MainWindow.cpp
  src/SynthPropertiesDialog.cpp
  src/SynthWidget.cpp
)

set(mt32emu_qt_HEADERS
  src/ALSADriver.h
  src/MainWindow.h
  src/SynthPropertiesDialog.h
  src/SynthWidget.h
)

set(mt32emu_qt_FORMS
  src/MainWindow.ui
  src/SynthWidget.ui
  src/SynthPropertiesDialog.ui
)

find_package(ALSA REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${ALSA_LIBRARIES})

find_package(Qt4 4.6.0 REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${QT_LIBRARIES})
include(${QT_USE_FILE})

find_package(QtMobility REQUIRED)
if(NOT QT_MOBILITY_MULTIMEDIAKIT_FOUND)
  message(FATAL_ERROR "QtMultimediaKit from QtMobility is required but could not be found - you may need to install qtmobility-dev")
endif()
set(EXT_LIBS ${EXT_LIBS} ${QT_MOBILITY_MULTIMEDIAKIT_LIBRARY})
include_directories(${QT_MOBILITY_MULTIMEDIAKIT_INCLUDE_DIR})
set(EXT_LIBS ${EXT_LIBS} ${QT_MOBILITY_LIBRARY})
include_directories(${QT_MOBILITY_INCLUDE_DIR})

find_package(MT32EMU REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${MT32EMU_LIBRARIES})
include_directories(${MT32EMU_INCLUDE_DIRS})

qt4_wrap_cpp(mt32emu_qt_HEADERS_MOC ${mt32emu_qt_HEADERS})
qt4_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(mt32emu-qt
  ${mt32emu_qt_HEADERS_MOC}
  ${mt32emu_qt_FORMS_HEADERS}
  ${mt32emu_qt_SOURCES}
)

target_link_libraries(mt32emu-qt
  ${QT_LIBRARIES}
  ${EXT_LIBS}
)

install(TARGETS
  mt32emu-qt
  DESTINATION bin
)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${mt32emu_qt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${mt32emu_qt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${mt32emu_qt_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Experimental Qt application using libmt32emu")
set(CPACK_PACKAGE_CONTACT "re_munt@kingguppy.com")
include(CPack)
