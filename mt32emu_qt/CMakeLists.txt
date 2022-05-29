cmake_minimum_required(VERSION 2.8.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(mt32emu-qt)

include(cmake/project_data.cmake)

if(BUILD_SNAPSHOTS AND libmt32emu_SNAPSHOT_VERSION)
  include(../cmake/build_snapshots.cmake)
  add_definitions(-DBUILD_MT32EMU_VERSION="${libmt32emu_SNAPSHOT_VERSION}")
endif()

include(GNUInstallDirs)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(LINUX_FOUND TRUE)
else()
  set(LINUX_FOUND FALSE)
endif()

set(mt32emu-qt_WITH_QT_VERSION AUTO CACHE STRING "Use the Qt library major version (supported range is 4..6); useful when multiple versions are available in the system")
set_property(CACHE mt32emu-qt_WITH_QT_VERSION PROPERTY STRINGS AUTO 4 5 6)

option(mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER "Use ALSA MIDI sequencer" ${LINUX_FOUND})
option(mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING "Load PulseAudio library dynamically" TRUE)
option(mt32emu-qt_WITH_DEBUG_WINCONSOLE "Use console for showing debug output on Windows" FALSE)

if(MSVC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(${PROJECT_NAME}_PRECOMPILED_HEADER "" CACHE FILEPATH "Header file to use as precompiled header, included before each source file")
  mark_as_advanced(${PROJECT_NAME}_PRECOMPILED_HEADER)
endif()

add_definitions(-DMT32EMU_QT_VERSION="${mt32emu_qt_VERSION}")
add_definitions(-DBUILD_SYSTEM="${CMAKE_SYSTEM_NAME}")

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
  src/QRingBuffer.cpp
  src/QMidiBuffer.cpp
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
  src/FloatingDisplay.cpp
  src/DemoPlayer.cpp

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
  list(APPEND mt32emu_qt_SOURCES
    src/mididrv/Win32Driver.cpp
    src/audiodrv/WinMMAudioDriver.cpp
  )
  list(APPEND EXT_LIBS winmm)
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
  set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework AudioToolbox -framework CoreAudio -framework CoreMIDI")
  set(CMAKE_MACOSX_BUNDLE True)
else()
  list(APPEND mt32emu_qt_SOURCES src/mididrv/OSSMidiPortDriver.cpp)
  list(APPEND EXT_LIBS pthread)
  if(OS2)
    list(APPEND EXT_LIBS cx)
  endif()
  CHECK_CXX_SYMBOL_EXISTS(clock_nanosleep time.h CLOCK_NANOSLEEP_FOUND)
  if(CLOCK_NANOSLEEP_FOUND)
    add_definitions(-DWITH_POSIX_CLOCK_NANOSLEEP)
    check_library_exists(rt clock_nanosleep "" CLOCK_NANOSLEEP_IN_RT_FOUND)
    if(CLOCK_NANOSLEEP_IN_RT_FOUND)
      list(APPEND EXT_LIBS rt)
    endif(CLOCK_NANOSLEEP_IN_RT_FOUND)
  endif(CLOCK_NANOSLEEP_FOUND)
endif()

if(mt32emu-qt_WITH_QT_VERSION EQUAL 6)
  find_package(Qt6Widgets REQUIRED CONFIG)
elseif(mt32emu-qt_WITH_QT_VERSION EQUAL 5)
  find_package(Qt5Widgets REQUIRED CONFIG)
elseif(mt32emu-qt_WITH_QT_VERSION EQUAL 4)
  find_package(Qt4 4.5.3 MODULE REQUIRED)
elseif(NOT DEFINED mt32emu-qt_WITH_QT5)
  find_package(Qt6Widgets CONFIG)
  if(NOT Qt6Widgets_FOUND)
    find_package(Qt5Widgets CONFIG)
    if(NOT Qt5Widgets_FOUND)
      find_package(Qt4 4.5.3 MODULE)
      if(NOT Qt4_FOUND)
        message(FATAL_ERROR "Not found any version of the Qt library")
      endif()
    endif()
  endif()
elseif(mt32emu-qt_WITH_QT5)
  find_package(Qt5Widgets CONFIG)
  if(NOT Qt5Widgets_FOUND)
    find_package(Qt4 4.5.3 MODULE REQUIRED)
  endif()
else()
  find_package(Qt4 4.5.3 MODULE)
  if(NOT Qt4_FOUND)
    find_package(Qt5Widgets REQUIRED CONFIG)
  endif()
endif()

if(Qt6Widgets_FOUND)
  list(APPEND EXT_LIBS Qt6::Widgets)
  find_package(Qt6Multimedia QUIET CONFIG)
  message(STATUS "Found Qt6Core version ${Qt6Core_VERSION}, C++11 required")
  set(mt32emu-qt_REQUIRED_CPP11 True)
elseif(Qt5Widgets_FOUND)
  list(APPEND EXT_LIBS Qt5::Widgets)
  find_package(Qt5Multimedia QUIET CONFIG)
  if(NOT(Qt5Core_VERSION VERSION_LESS 5.7.0))
    message(STATUS "Found Qt5Core version ${Qt5Core_VERSION}, C++11 required")
    set(mt32emu-qt_REQUIRED_CPP11 True)
  endif()
else()
  list(APPEND EXT_LIBS Qt4::QtGui)
endif()

if(QT_QTMULTIMEDIA_FOUND OR Qt5Multimedia_FOUND OR Qt6Multimedia_FOUND)
  add_definitions(-DWITH_QT_AUDIO_DRIVER)
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/QtAudioDriver.cpp)
  if(Qt6Multimedia_FOUND)
    list(APPEND EXT_LIBS Qt6::Multimedia)
  elseif(Qt5Multimedia_FOUND)
    list(APPEND EXT_LIBS Qt5::Multimedia)
  else()
    list(APPEND EXT_LIBS Qt4::QtMultimedia)
  endif()
elseif(Qt4_FOUND)
  find_package(QtMobility MODULE)
  if(QT_MOBILITY_MULTIMEDIAKIT_FOUND)
    add_definitions(-DWITH_QT_AUDIO_DRIVER -DUSE_QT_MULTIMEDIAKIT)
    include_directories(${QT_MOBILITY_MULTIMEDIAKIT_INCLUDE_DIR} ${QT_MOBILITY_INCLUDE_DIR})
    list(APPEND mt32emu_qt_SOURCES src/audiodrv/QtAudioDriver.cpp)
    list(APPEND EXT_LIBS ${QT_MOBILITY_MULTIMEDIAKIT_LIBRARY})
  endif()
endif()

if(NOT(munt_SOURCE_DIR AND TARGET MT32Emu::mt32emu))
  find_package(MT32Emu 2.7 CONFIG REQUIRED)
endif()
list(APPEND EXT_LIBS MT32Emu::mt32emu)

find_package(PORTAUDIO 19 MODULE)
if(PORTAUDIO_FOUND)
  add_definitions(-DWITH_PORT_AUDIO_DRIVER)
  list(APPEND EXT_LIBS portaudio)
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/PortAudioDriver.cpp)
endif()

find_package(ALSA MODULE)
if(ALSA_FOUND)
  add_definitions(-DWITH_ALSA_AUDIO_DRIVER)
  list(APPEND EXT_LIBS ${ALSA_LIBRARIES})
  include_directories(${ALSA_INCLUDE_DIRS})
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/AlsaAudioDriver.cpp)
  if(mt32emu-qt_WITH_ALSA_MIDI_SEQUENCER)
    add_definitions(-DWITH_ALSA_MIDI_DRIVER)
    list(APPEND mt32emu_qt_SOURCES src/mididrv/ALSADriver.cpp)
  endif()
endif()

find_package(PulseAudio MODULE)
if(PulseAudio_FOUND)
  add_definitions(-DWITH_PULSE_AUDIO_DRIVER)
  if(mt32emu-qt_USE_PULSEAUDIO_DYNAMIC_LOADING)
    add_definitions(-DUSE_PULSEAUDIO_DYNAMIC_LOADING)
    check_library_exists(dl dlopen "" SEPARATE_DL_FOUND)
    if(SEPARATE_DL_FOUND)
      list(APPEND EXT_LIBS dl)
    endif()
  else()
    list(APPEND EXT_LIBS PulseAudio::simple)
  endif()
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/PulseAudioDriver.cpp)
endif()

check_include_files(sys/soundcard.h SYS_SOUNDCARD_FOUND)
if(SYS_SOUNDCARD_FOUND)
  add_definitions(-DWITH_OSS_AUDIO_DRIVER)
  list(APPEND mt32emu_qt_SOURCES src/audiodrv/OSSAudioDriver.cpp)
endif()

find_package(JACK MODULE)
if(JACK_FOUND)
  add_definitions(-DWITH_JACK_MIDI_DRIVER -DWITH_JACK_AUDIO_DRIVER)
  list(APPEND EXT_LIBS Jack::jack)
  include_directories(${JACK_INCLUDE_DIRS})
  list(APPEND mt32emu_qt_SOURCES
    src/JACKClient.cpp
    src/mididrv/JACKMidiDriver.cpp
    src/audiodrv/JACKAudioDriver.cpp
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
elseif(Qt5Widgets_FOUND)
  qt5_add_resources(mt32emu_qt_RC ${mt32emu_qt_RESOURCES})
  qt5_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})
else()
  qt6_add_resources(mt32emu_qt_RC ${mt32emu_qt_RESOURCES})
  qt6_wrap_ui(mt32emu_qt_FORMS_HEADERS ${mt32emu_qt_FORMS})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(${PROJECT_NAME}_PRECOMPILED_HEADER AND CMAKE_VERSION VERSION_LESS 3.16)
  set(${PROJECT_NAME}_PRECOMPILED_HEADER_SRC ${CMAKE_CURRENT_BINARY_DIR}/pch.cpp)
  file(APPEND ${${PROJECT_NAME}_PRECOMPILED_HEADER_SRC} "")

  if(MSVC)
    set(${PROJECT_NAME}_PRECOMPILED_HEADER_PCH ${CMAKE_CURRENT_BINARY_DIR}/pch.pch)

    if(MSVC_IDE)
      add_compile_options(-Yu${${PROJECT_NAME}_PRECOMPILED_HEADER})
    endif()

    add_compile_options(-FI${${PROJECT_NAME}_PRECOMPILED_HEADER} -Fp${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH})
    set_source_files_properties(pch.cpp
      PROPERTIES
        COMPILE_FLAGS -Yc"${${PROJECT_NAME}_PRECOMPILED_HEADER}"
        OBJECT_DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER}
        OBJECT_OUTPUTS ${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH}
        SKIP_AUTOMOC TRUE
    )

    if(NOT MSVC_IDE)
      set_source_files_properties(${mt32emu_qt_SOURCES}
        PROPERTIES
          COMPILE_FLAGS -Yu${${PROJECT_NAME}_PRECOMPILED_HEADER}
          OBJECT_DEPENDS ${${PROJECT_NAME}_PRECOMPILED_HEADER_PCH}
      )
    endif()
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
endif(${PROJECT_NAME}_PRECOMPILED_HEADER AND CMAKE_VERSION VERSION_LESS 3.16)

add_executable(mt32emu-qt
  ${${PROJECT_NAME}_PRECOMPILED_HEADER_SRC}
  ${mt32emu_qt_FORMS_HEADERS}
  ${mt32emu_qt_SOURCES}
  ${mt32emu_qt_RC}
)

if(${PROJECT_NAME}_PRECOMPILED_HEADER)
  if(NOT CMAKE_VERSION VERSION_LESS 3.16)
    target_precompile_headers(mt32emu-qt
      PRIVATE ${${PROJECT_NAME}_PRECOMPILED_HEADER}
    )
  elseif(XCODE)
    set_target_properties(${PROJECT_NAME}
      PROPERTIES
        XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
        XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${${PROJECT_NAME}_PRECOMPILED_HEADER}"
    )
  endif(NOT CMAKE_VERSION VERSION_LESS 3.16)
endif(${PROJECT_NAME}_PRECOMPILED_HEADER)

target_link_libraries(mt32emu-qt PRIVATE ${EXT_LIBS})

if(WIN32)
  set_target_properties(mt32emu-qt
    PROPERTIES VERSION ${mt32emu_qt_VERSION}
  )
elseif(APPLE)
  set_target_properties(mt32emu-qt
    PROPERTIES
      MACOSX_BUNDLE_BUNDLE_VERSION ${mt32emu_qt_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${mt32emu_qt_VERSION}
  )
endif()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)

install(TARGETS
  mt32emu-qt
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
  install(FILES res/mt32emu-qt.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
  foreach(${PROJECT_NAME}_ICON_RESOLUTION 16x16;24x24;32x32;48x48;64x64;72x72;96x96;128x128;256x256)
    install(FILES res/icons/${${PROJECT_NAME}_ICON_RESOLUTION}/munt.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${${PROJECT_NAME}_ICON_RESOLUTION}/apps)
    # These directories pre-exist on a freedesktop-conforming system and cause conflict if not excluded
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/${${PROJECT_NAME}_ICON_RESOLUTION})
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/${${PROJECT_NAME}_ICON_RESOLUTION}/apps)
  endforeach(${PROJECT_NAME}_ICON_RESOLUTION)
endif(UNIX AND NOT APPLE)

install(FILES
  AUTHORS.txt COPYING.txt NEWS.txt README.md TODO.txt
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/munt/${PROJECT_NAME}
)

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${mt32emu_qt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${mt32emu_qt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${mt32emu_qt_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "${mt32emu_qt_CONTACT}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/info.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${mt32emu_qt_DESCRIPTION_SUMMARY}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)
# The default package file name has "-${CPACK_SYSTEM_NAME}" at the end, which doesn't match our previous releases
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${mt32emu_qt_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_PACKAGE_EXECUTABLES mt32emu-qt;Munt\ MT-32\ Sound\ Module\ Emulator)
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RPM_PACKAGE_URL "${mt32emu_qt_URL}")
# These directories pre-exist on a freedesktop-conforming system and cause conflict if not excluded
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
  ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications
  ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons
  ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor
)
if(munt_SOURCE_DIR)
  # Also publish the list for the benefit of the parent CMakeLists.txt file.
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION} PARENT_SCOPE)
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${mt32emu_qt_URL}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
