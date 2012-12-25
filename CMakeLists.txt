cmake_minimum_required(VERSION 2.6)
project(munt)
set(libmt32emu_VERSION_MAJOR 1)
set(libmt32emu_VERSION_MINOR 0)
set(libmt32emu_VERSION_PATCH 0)

option(WITH_INTERNAL_PORTAUDIO "Use internal version of PortAudio" TRUE)

if(WITH_INTERNAL_PORTAUDIO)
  add_subdirectory(portaudio)
endif()

add_subdirectory(mt32emu)
add_subdirectory(mt32emu_smf2wav)
add_subdirectory(mt32emu_qt)

add_dependencies(mt32emu-smf2wav mt32emu)
add_dependencies(mt32emu-qt mt32emu)

if(WITH_INTERNAL_PORTAUDIO)
  add_dependencies(mt32emu-qt portaudio)
endif()
