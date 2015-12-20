cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-qt)
set(mt32emu_qt_VERSION_MAJOR 1)
set(mt32emu_qt_VERSION_MINOR 3)
set(mt32emu_qt_VERSION_PATCH 0)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(LINUX_FOUND TRUE)
else()
  set(LINUX_FOUND FALSE)
endif()

option(mt32emu-qt_WITH_QT5 "Prefer Qt5 if Qt4 is also available" FALSE)
option(mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER "Use ALSA MIDI sequencer" ${LINUX_FOUND})
option(mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING "Load PulseAudio library dynamically" TRUE)
option(mt32emu-qt_WITH_DEBUG_WINCONSOLE "Use console for showing debug output on Windows" FALSE)
option(mt32emu-qt_WITH_INTERNAL_RESAMPLER "Use internal resampler" TRUE)

add_definitions(-DPACKAGE_VERSION="${munt_VERSION_MAJOR}.${munt_VERSION_MINOR}.${munt_VERSION_PATCH}")
add_definitions(-DAPP_VERSION="${mt32emu_qt_VERSION_MAJOR}.${mt32emu_qt_VERSION_MINOR}.${mt32emu_qt_VERSION_PATCH}")
add_definitions(-DBUILD_SYSTEM="${CMAKE_SYSTEM_NAME}")

if(libmt32emu_SHARED)
  add_definitions(-DMT32EMU_SHARED)
endif()

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

  src/resample/SampleRateConverter.cpp
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

set(mt32emu_qt_RESOURCES
  src/images.qrc
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CYGWIN)
  add_definitions(-DWITH_WIN32_MIDI_DRIVER -DWITH_WINMM_AUDIO_DRIVER -DWITH_WINMMTIMER)
  list(APPEND mt32emu_qt_SOURCES src/mididrv/Win32Driver.cpp)
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/WinMMAudioDriver.cpp)
  set(EXT_LIBS ${EXT_LIBS} winmm)
  if(NOT CYGWIN AND mt32emu-qt_WITH_DEBUG_WINCONSOLE)
    add_definitions(-DWITH_WINCONSOLE)
  else()
    set(CMAKE_WIN32_EXECUTABLE True)
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_definitions(-DWITH_COREMIDI_DRIVER -DWITH_COREAUDIO_DRIVER)
  list(APPEND mt32emu_qt_SOURCES
    src/mididrv/CoreMidiDriver.cpp
    src/audiodrv/CoreAudioDriver.cpp
  )
  set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework AudioToolbox -framework CoreMIDI")
  set(CMAKE_MACOSX_BUNDLE True)
else()
  list(APPEND mt32emu_qt_SOURCES src/mididrv/OSSMidiPortDriver.cpp)
  set(EXT_LIBS ${EXT_LIBS} pthread)
  check_library_exists(rt clock_nanosleep "" NANOSLEEP_IN_RT_FOUND)
  if(NANOSLEEP_IN_RT_FOUND)
    set(EXT_LIBS ${EXT_LIBS} rt)
  endif()
endif()

if(NOT mt32emu-qt_WITH_QT5)
  find_package(Qt4 4.8.0)
endif()
if(Qt4_FOUND)
  set(EXT_LIBS ${EXT_LIBS} Qt4::QtGui)
else()
  find_package(Qt5Widgets REQUIRED)
  set(EXT_LIBS ${EXT_LIBS} Qt5::Widgets)
  find_package(Qt5Multimedia)
endif()

if(QT_QTMULTIMEDIA_FOUND OR Qt5Multimedia_FOUND)
  add_definitions(-DWITH_QT_AUDIO_DRIVER)
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/QtAudioDriver.cpp
  )
  if(Qt5Multimedia_FOUND)
    set(EXT_LIBS ${EXT_LIBS} Qt5::Multimedia)
  else()
    set(EXT_LIBS ${EXT_LIBS} Qt4::QtMultimedia)
  endif()
else()
  find_package(QtMobility)
  if(QT_MOBILITY_MULTIMEDIAKIT_FOUND)
    add_definitions(-DWITH_QT_AUDIO_DRIVER -DUSE_QT_MULTIMEDIAKIT)
    include_directories(${QT_MOBILITY_MULTIMEDIAKIT_INCLUDE_DIR} ${QT_MOBILITY_INCLUDE_DIR})
    set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
      src/audiodrv/QtAudioDriver.cpp
    )
    set(EXT_LIBS ${EXT_LIBS} ${QT_MOBILITY_MULTIMEDIAKIT_LIBRARY})
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
endif()

find_package(ALSA)
if(ALSA_FOUND)
  add_definitions(-DWITH_ALSA_AUDIO_DRIVER)
  set(EXT_LIBS ${EXT_LIBS} ${ALSA_LIBRARIES})
  include_directories(${ALSA_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/AlsaAudioDriver.cpp
  )
  if(mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER)
    add_definitions(-DWITH_ALSA_MIDI_DRIVER)
    set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
      src/mididrv/ALSADriver.cpp
    )
  endif()
endif()

find_package(PulseAudio)
if(PULSEAUDIO_FOUND)
  add_definitions(-DWITH_PULSE_AUDIO_DRIVER)
  if(mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING)
    add_definitions(-DUSE_PULSEAUDIO_DYNAMIC_LOADING)
    check_library_exists(dl dlopen "" SEPARATE_DL_FOUND)
    if(SEPARATE_DL_FOUND)
      set(EXT_LIBS ${EXT_LIBS} dl)
    endif()
  else()
    set(EXT_LIBS ${EXT_LIBS} ${PULSEAUDIO_LIBRARIES})
  endif()
  include_directories(${PULSEAUDIO_INCLUDE_DIRS})
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/PulseAudioDriver.cpp
  )
endif()

check_include_files(sys/soundcard.h SYS_SOUNDCARD_FOUND)
if(SYS_SOUNDCARD_FOUND)
  add_definitions(-DWITH_OSS_AUDIO_DRIVER)
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/audiodrv/OSSAudioDriver.cpp
  )
endif()

if(NOT mt32emu-qt_WITH_INTERNAL_RESAMPLER)
  # Prefer using SOXR if it is available
  find_package(LibSoxr)
  if(LIBSOXR_FOUND)
    add_definitions(-DWITH_LIBSOXR_RESAMPLER)
    set(EXT_LIBS ${EXT_LIBS} ${LIBSOXR_LIBRARY})
    include_directories(${LIBSOXR_INCLUDE_DIR})
    set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
      src/resample/SoxrAdapter.cpp
    )
  else(LIBSOXR_FOUND)
    message(STATUS "Could NOT find libsoxr")

    # Try Samplerate if SOXR is unavailable
    find_package(LibSamplerate)
    if(LIBSAMPLERATE_FOUND)
      add_definitions(-DWITH_LIBSAMPLERATE_RESAMPLER)
      set(EXT_LIBS ${EXT_LIBS} ${LIBSAMPLERATE_LIBRARY})
      include_directories(${LIBSAMPLERATE_INCLUDE_DIR})
      set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
        src/resample/SamplerateAdapter.cpp
      )
    else(LIBSAMPLERATE_FOUND)
      message(STATUS "Could NOT find libsamplerate")
      set(mt32emu-qt_WITH_INTERNAL_RESAMPLER "TRUE")
    endif(LIBSAMPLERATE_FOUND)
  endif(LIBSOXR_FOUND)
endif(NOT mt32emu-qt_WITH_INTERNAL_RESAMPLER)

if(mt32emu-qt_WITH_INTERNAL_RESAMPLER)
  set(mt32emu_qt_SOURCES ${mt32emu_qt_SOURCES}
    src/resample/FIRInterpolator.cpp
    src/resample/SincResampler.cpp
    src/resample/IIRDecimator.cpp
    src/resample/InternalResampler.cpp
    src/resample/LinearResampler.cpp
  )
endif(mt32emu-qt_WITH_INTERNAL_RESAMPLER)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -ansi)
endif()

if(Qt4_FOUND)
  qt4_add_resources(mt32emu_qt_RC ${mt32emu_qt_RESOURCES})
  qt4_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})
else()
  qt5_add_resources(mt32emu_qt_RC ${mt32emu_qt_RESOURCES})
  qt5_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(mt32emu-qt
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

install(FILES
  AUTHORS.txt COPYING.txt NEWS.txt README.txt TODO.txt
  DESTINATION share/doc/munt/mt32emu-qt
)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${munt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${munt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${munt_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Main MT-32 emulation application using libmt32emu")
set(CPACK_PACKAGE_CONTACT "re_munt@kingguppy.com")
set(CPACK_PACKAGE_EXECUTABLES mt32emu-qt;Munt\ MT-32\ Sound\ Module\ Emulator)
include(CPack)
