cmake_minimum_required(VERSION 2.6)
project(munt)
set(munt_VERSION_MAJOR 1)
set(munt_VERSION_MINOR 4)
set(munt_VERSION_PATCH 0)

option(munt_WITH_MT32EMU_SMF2WAV "Build command line standard MIDI file conversion tool" TRUE)
option(munt_WITH_MT32EMU_QT "Build Qt-based UI-enabled application" TRUE)
option(munt_WITH_INTERNAL_PORTAUDIO "Use internal version of PortAudio" TRUE)

if(munt_WITH_MT32EMU_QT AND munt_WITH_INTERNAL_PORTAUDIO)
  add_subdirectory(portaudio)
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

if(munt_WITH_MT32EMU_QT AND munt_WITH_INTERNAL_PORTAUDIO)
  add_dependencies(mt32emu-qt portaudio)
endif()
