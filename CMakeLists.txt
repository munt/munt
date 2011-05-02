cmake_minimum_required(VERSION 2.6)

add_subdirectory(mt32emu)
add_subdirectory(mt32emu_smf2wav)

add_dependencies(mt32emu-smf2wav libmt32emu)
