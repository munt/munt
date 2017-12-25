cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-qt)
set(mt32emu_qt_VERSION_MAJOR 1)
set(mt32emu_qt_VERSION_MINOR 7)
set(mt32emu_qt_VERSION_PATCH 1)
set(mt32emu_qt_VERSION "${mt32emu_qt_VERSION_MAJOR}.${mt32emu_qt_VERSION_MINOR}.${mt32emu_qt_VERSION_PATCH}")

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(LINUX_FOUND TRUE)
else()
  set(LINUX_FOUND FALSE)
endif()

option(mt32emu-qt_WITH_QT5 "Prefer Qt5 if Qt4 is also available (prefer Qt4 otherwise)" TRUE)
option(mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER "Use ALSA MIDI sequencer" ${LINUX_FOUND})
option(mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING "Load PulseAudio library dynamically" TRUE)
option(mt32emu-qt_WITH_DEBUG_WINCONSOLE "Use console for showing debug output on Windows" FALSE)

if(MSVC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(${PROJECT_NAME}_PRECOMPILED_HEADER "" CACHE FILEPATH "Header file to use as precompiled header, included before each source file")
  mark_as_advanced(${PROJECT_NAME}_PRECOMPILED_HEADER)
endif()

add_definitions(-DAPP_VERSION="${mt32emu_qt_VERSION}")
add_definitions(-DBUILD_SYSTEM="${CMAKE_SYSTEM_NAME}")

if(libmt32emu_SHARED)
  add_definitions(-DMT32EMU_SHARED)
endif()

include(CheckIncludeFiles)
include(CheckCXXSymbolExists)
include(CheckLibraryExists)

set(mt32emu_qt_SOURCES
  src/main.cpp

  src/AudioFileWriter.cpp
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
  CHECK_CXX_SYMBOL_EXISTS(clock_nanosleep time.h CLOCK_NANOSLEEP_FOUND)
  if(CLOCK_NANOSLEEP_FOUND)
    add_definitions(-DWITH_POSIX_CLOCK_NANOSLEEP)
    check_library_exists(rt clock_nanosleep "" CLOCK_NANOSLEEP_IN_RT_FOUND)
    if(CLOCK_NANOSLEEP_IN_RT_FOUND)
      set(EXT_LIBS ${EXT_LIBS} rt)
    endif(CLOCK_NANOSLEEP_IN_RT_FOUND)
  endif(CLOCK_NANOSLEEP_FOUND)
endif()

if(mt32emu-qt_WITH_QT5)
  find_package(Qt5Widgets)
  if(NOT Qt5Widgets_FOUND)
    find_package(Qt4 4.5.3 REQUIRED)
  endif()
else(mt32emu-qt_WITH_QT5)
  find_package(Qt4 4.5.3)
  if(NOT Qt4_FOUND)
    find_package(Qt5Widgets REQUIRED)
  endif()
endif(mt32emu-qt_WITH_QT5)

if(Qt5Widgets_FOUND)
  set(EXT_LIBS ${EXT_LIBS} Qt5::Widgets)
  find_package(Qt5Multimedia QUIET)
  if(NOT(Qt5Core_VERSION VERSION_LESS 5.7.0))
    message(STATUS "Found Qt5Core version ${Qt5Core_VERSION}, C++11 required")
    set(mt32emu-qt_REQUIRED_CPP11 True)
  endif()
else(Qt5Widgets_FOUND)
  set(EXT_LIBS ${EXT_LIBS} Qt4::QtGui)
endif(Qt5Widgets_FOUND)

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

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  add_compile_options(-Wall -Wextra -Wnon-virtual-dtor)
  if(mt32emu-qt_REQUIRED_CPP11)
    if(CMAKE_VERSION VERSION_LESS 3.1.0)
      add_compile_options(-std=c++11)
    endif()
  else(mt32emu-qt_REQUIRED_CPP11)
    add_compile_options(-ansi)
  endif(mt32emu-qt_REQUIRED_CPP11)
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

if(${PROJECT_NAME}_PRECOMPILED_HEADER)
  set(${PROJECT_NAME}_PRECOMPILED_HEADER_SRC ${CMAKE_CURRENT_BINARY_DIR}/pch.cpp)
  file(APPEND ${${PROJECT_NAME}_PRECOMPILED_HEADER_SRC} "")

  if(MSVC)
    set(${PROJECT_NAME}_PRECOMPILED_HEADER_PCH ${CMAKE_CURRENT_BINARY_DIR}/pch.pch)

    add_compile_options(-Yu${${PROJECT_NAME}_PRECOMPILED_HEADER} -FI${${PROJECT_NAME}_PRECOMPILED_HEADER} -Fp${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH})
    set_source_files_properties(pch.cpp
      PROPERTIES
        COMPILE_FLAGS -Yc"${${PROJECT_NAME}_PRECOMPILED_HEADER}"
        OBJECT_DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER}
        SKIP_AUTOMOC TRUE
    )

    # This is only necessary for NMake generator
    set_source_files_properties(${mt32emu_qt_SOURCES}
      PROPERTIES
        OBJECT_DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER}
    )
  elseif(NOT XCODE)
    add_compile_options(-Winvalid-pch)

    # We could generate PCH file right from the specified header but it turns out awkward to compile within cmake.
    # There are various quite complex solutions around to overcome this issue. But it seems to be much easier to just compile
    # an empty .cpp file while setting the language to treat it as a header.
    set_source_files_properties(pch.cpp
      PROPERTIES
        COMPILE_FLAGS "-x c++-header -include ${${PROJECT_NAME}_PRECOMPILED_HEADER}"
        OBJECT_DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER}
        SKIP_AUTOMOC TRUE
    )

    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      set(${PROJECT_NAME}_OSX_DEPLOYMENT_TARGET_FLAG -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    # We also need to exclude generated PCH file from the link phase. Instead of creating another target and synchronising
    # a lot of compile properties, we simply compile our empty .cpp file and use the output for linking.
    set(${PROJECT_NAME}_PRECOMPILED_HEADER_PCH ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}.dir/pch.cpp${CMAKE_CXX_OUTPUT_EXTENSION})
    add_custom_command(OUTPUT pch.h.gch
      COMMAND ${CMAKE_COMMAND} -E rename ${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH} pch.h.gch
      COMMAND ${CMAKE_CXX_COMPILER} -c ${${PROJECT_NAME}_OSX_DEPLOYMENT_TARGET_FLAG} -o ${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH} ${${PROJECT_NAME}_PRECOMPILED_HEADER_SRC}
      COMMAND ${CMAKE_COMMAND} -E touch pch.h.gch
      DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH}
    )

    set_source_files_properties(${mt32emu_qt_SOURCES}
      PROPERTIES
        COMPILE_FLAGS "-include pch.h"
        OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pch.h.gch
    )
  endif(MSVC)
endif(${PROJECT_NAME}_PRECOMPILED_HEADER)

add_executable(mt32emu-qt
  ${${PROJECT_NAME}_PRECOMPILED_HEADER_SRC}
  ${mt32emu_qt_FORMS_HEADERS}
  ${mt32emu_qt_SOURCES}
  ${mt32emu_qt_RC}
)

if(${PROJECT_NAME}_PRECOMPILED_HEADER AND XCODE)
  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
      XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${${PROJECT_NAME}_PRECOMPILED_HEADER}"
  )
endif()

target_link_libraries(mt32emu-qt
  ${EXT_LIBS}
)

if(WIN32)
  set_target_properties(mt32emu-qt
    PROPERTIES VERSION ${mt32emu_qt_VERSION}
  )
endif()

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
set(CPACK_PACKAGE_VERSION_MAJOR "${mt32emu_qt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${mt32emu_qt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${mt32emu_qt_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "sergm@muntemu.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../FreeBSD/usr/ports/audio/mt32emu_qt/pkg-descr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Main MT-32 emulation application using libmt32emu")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_EXECUTABLES mt32emu-qt;Munt\ MT-32\ Sound\ Module\ Emulator)
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_URL "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
