cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-qt)
set(mt32emu_qt_VERSION_MAJOR 0)
set(mt32emu_qt_VERSION_MINOR 1)
set(mt32emu_qt_VERSION_PATCH 0)

set(mt32emu_qt_SOURCES
  src/MidiEvent.cpp
  src/MidiEventQueue.cpp
  src/QSynth.cpp
  src/main.cpp
  src/MainWindow.cpp
  src/SynthManager.cpp
  src/SynthPropertiesDialog.cpp
  src/SynthWidget.cpp

  src/audiodrv/AudioDriver.cpp

  src/mididrv/MidiDriver.cpp
  src/mididrv/TestDriver.cpp
#  src/mididrv/Win32Driver.cpp
)

set(mt32emu_qt_HEADERS
  src/mididrv/MidiDriver.h
  src/mididrv/TestDriver.h
  src/MainWindow.h
  src/SynthManager.h
  src/SynthPropertiesDialog.h
  src/SynthWidget.h
  src/QSynth.h
)

set(mt32emu_qt_FORMS
  src/MainWindow.ui
  src/SynthWidget.ui
  src/SynthPropertiesDialog.ui
)

find_package(Qt4 4.6.0 REQUIRED)
include(${QT_USE_FILE})
set(EXT_LIBS ${EXT_LIBS} ${QT_LIBRARIES})

find_package(QtMobility)
if(QT_MOBILITY_MULTIMEDIAKIT_FOUND)
  set(EXT_LIBS ${EXT_LIBS} ${QT_MOBILITY_MULTIMEDIAKIT_LIBRARY})
  include_directories(${QT_MOBILITY_MULTIMEDIAKIT_INCLUDE_DIR} ${QT_MOBILITY_INCLUDE_DIR})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/QtAudioDriver.cpp
  )
endif()

find_package(MT32EMU REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${MT32EMU_LIBRARIES})
include_directories(${MT32EMU_INCLUDE_DIRS})

find_package(ALSA)
if(ALSA_FOUND)
  set(EXT_LIBS ${EXT_LIBS} ${ALSA_LIBRARIES})
  include_directories(${ALSA_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/mididrv/ALSADriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
    src/mididrv/ALSADriver.h
  )
endif()

find_package(PORTAUDIO)
if(PORTAUDIO_FOUND)
  set(EXT_LIBS ${EXT_LIBS} ${PORTAUDIO_LIBRARIES})
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/PortAudioDriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
    src/audiodrv/PortAudioDriver.h
  )
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -ansi)
endif()

qt4_wrap_cpp(mt32emu_qt_HEADERS_MOC ${mt32emu_qt_HEADERS})
qt4_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(mt32emu-qt
  ${mt32emu_qt_HEADERS_MOC}
  ${mt32emu_qt_FORMS_HEADERS}
  ${mt32emu_qt_SOURCES}
)

target_link_libraries(mt32emu-qt
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
