cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-qt)
set(mt32emu_qt_VERSION_MAJOR 1)
set(mt32emu_qt_VERSION_MINOR 0)
set(mt32emu_qt_VERSION_PATCH 2)

add_definitions(-DVERSION="${mt32emu_qt_VERSION_MAJOR}.${mt32emu_qt_VERSION_MINOR}.${mt32emu_qt_VERSION_PATCH}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

set(mt32emu_qt_SOURCES
  src/main.cpp

  src/AudioFileWriter.cpp
  src/ClockSync.cpp
  src/MainWindow.cpp
  src/Master.cpp
  src/MasterClock.cpp
  src/MidiParser.cpp
  src/MidiRecorder.cpp
  src/MidiSession.cpp
  src/QMidiEvent.cpp
  src/QSynth.cpp
  src/SynthRoute.cpp
  src/SynthPropertiesDialog.cpp
  src/AudioPropertiesDialog.cpp
  src/MidiConverterDialog.cpp
  src/MidiPropertiesDialog.cpp
  src/ROMSelectionDialog.cpp
  src/SynthStateMonitor.cpp
  src/SynthWidget.cpp
  src/MidiPlayerDialog.cpp

  src/audiodrv/AudioDriver.cpp
  src/audiodrv/AudioFileWriterDriver.cpp

  src/mididrv/MidiDriver.cpp
  src/mididrv/TestDriver.cpp
  src/mididrv/SMFDriver.cpp
)

set(mt32emu_qt_RESOURCES
  src/images.qrc
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  add_definitions(-DWITH_WIN32_MIDI_DRIVER -DWITH_WINMM_AUDIO_DRIVER -DWITH_WINMMTIMER -DWITH_WINCONSOLE)
  list(APPEND mt32emu_qt_SOURCES src/mididrv/Win32Driver.cpp)
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/WinMMAudioDriver.cpp)
  set(EXT_LIBS ${EXT_LIBS} winmm)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_definitions(-DWITH_COREMIDI_DRIVER -DWITH_COREAUDIO_DRIVER)
  list(APPEND mt32emu_qt_SOURCES src/mididrv/CoreMidiDriver.cpp)
  set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreMIDI")
else()
  list(APPEND mt32emu_qt_SOURCES src/mididrv/OSSMidiPortDriver.cpp)
  set(EXT_LIBS ${EXT_LIBS} pthread)
  check_library_exists(rt clock_nanosleep "" NANOSLEEP_IN_RT_FOUND)
  if(NANOSLEEP_IN_RT_FOUND)
    set(EXT_LIBS ${EXT_LIBS} rt)
  endif()
endif()

set(mt32emu_qt_HEADERS
  src/AudioFileWriter.h
  src/MainWindow.h
  src/Master.h
  src/MidiSession.h
  src/QSynth.h
  src/SynthPropertiesDialog.h
  src/AudioPropertiesDialog.h
  src/MidiConverterDialog.h
  src/MidiPropertiesDialog.h
  src/ROMSelectionDialog.h
  src/SynthRoute.h
  src/SynthStateMonitor.h
  src/SynthWidget.h
  src/MidiPlayerDialog.h

#  src/audiodrv/AudioDriver.h

  src/mididrv/MidiDriver.h
  src/mididrv/TestDriver.h
  src/mididrv/SMFDriver.h
)

set(mt32emu_qt_FORMS
  src/MainWindow.ui
  src/AudioPropertiesDialog.ui
  src/MidiPropertiesDialog.ui
  src/SynthPropertiesDialog.ui
  src/ROMSelectionDialog.ui
  src/SynthWidget.ui
  src/MidiConverterDialog.ui
  src/MidiPlayerDialog.ui
)

find_package(Qt4 4.6.0 REQUIRED)
include(${QT_USE_FILE})
set(EXT_LIBS ${EXT_LIBS} ${QT_LIBRARIES})

if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
  find_package(QtMobility)
  if(QT_QTMULTIMEDIA_FOUND)
    add_definitions(-DWITH_QT_AUDIO_DRIVER)
    set(EXT_LIBS ${EXT_LIBS} ${QT_QTMULTIMEDIA_LIBRARY})
    set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
      src/audiodrv/QtAudioDriver.cpp
    )
  elseif(QT_MOBILITY_MULTIMEDIAKIT_FOUND)
    add_definitions(-DWITH_QT_AUDIO_DRIVER -DUSE_QT_MULTIMEDIAKIT)
    set(EXT_LIBS ${EXT_LIBS} ${QT_MOBILITY_MULTIMEDIAKIT_LIBRARY})
    include_directories(${QT_MOBILITY_MULTIMEDIAKIT_INCLUDE_DIR} ${QT_MOBILITY_INCLUDE_DIR})
    set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
      src/audiodrv/QtAudioDriver.cpp
    )
  endif()
endif()

find_package(MT32EMU REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${MT32EMU_LIBRARIES})
include_directories(${MT32EMU_INCLUDE_DIRS})

find_package(PORTAUDIO)
if(PORTAUDIO_FOUND)
  add_definitions(-DWITH_PORT_AUDIO_DRIVER)
  set(EXT_LIBS ${EXT_LIBS} ${PORTAUDIO_LIBRARIES})
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/PortAudioDriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
#    src/audiodrv/PortAudioDriver.h
  )
endif()

find_package(ALSA)
if(ALSA_FOUND)
  add_definitions(-DWITH_ALSA_AUDIO_DRIVER -DWITH_ALSA_MIDI_DRIVER)
  set(EXT_LIBS ${EXT_LIBS} ${ALSA_LIBRARIES})
  include_directories(${ALSA_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/mididrv/ALSADriver.cpp
    src/audiodrv/AlsaAudioDriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
    src/mididrv/ALSADriver.h
#    src/audiodrv/AlsaAudioDriver.h
  )
endif()

find_package(PulseAudio)
if(PULSEAUDIO_FOUND)
  add_definitions(-DWITH_PULSE_AUDIO_DRIVER)
# FIXME: we need to setup a define for Pulse dynamic loading
# set(EXT_LIBS ${EXT_LIBS} ${PULSEAUDIO_LIBRARIES})
  check_library_exists(dl dlopen "" SEPARATE_DL_FOUND)
  if(SEPARATE_DL_FOUND)
    set(EXT_LIBS ${EXT_LIBS} dl)
  endif()
  include_directories(${PULSEAUDIO_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/PulseAudioDriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
#    src/audiodrv/PulseAudioDriver.h
  )
endif()

check_include_files(sys/soundcard.h SYS_SOUNDCARD_FOUND)
if(SYS_SOUNDCARD_FOUND)
  add_definitions(-DWITH_OSS_AUDIO_DRIVER)
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/OSSAudioDriver.cpp
  )
  set(mt32emu_qt_HEADERS ${mt32emu_qt_HEADERS}
#    src/audiodrv/OSSAudioDriver.h
  )
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -ansi)
endif()

QT4_ADD_RESOURCES(mt32emu_qt_RC ${mt32emu_qt_RESOURCES})
qt4_wrap_cpp(mt32emu_qt_HEADERS_MOC ${mt32emu_qt_HEADERS})
qt4_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(mt32emu-qt
  ${mt32emu_qt_HEADERS_MOC}
  ${mt32emu_qt_FORMS_HEADERS}
  ${mt32emu_qt_SOURCES}
  ${mt32emu_qt_RC}
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
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Main MT-32 emulation application using libmt32emu")
set(CPACK_PACKAGE_CONTACT "re_munt@kingguppy.com")
set(CPACK_PACKAGE_EXECUTABLES mt32emu-qt;Munt\ MT-32\ Sound\ Module\ Emulator)
include(CPack)
