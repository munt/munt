# Generates FreeBSD ports for the library mt32emu and applications mt32emu-qt and mt32emu-smf2wav.
#
# Variables that configure generation of the ports:
#  PORTS_DIR - the target directory where the ports directories will be written to. By default,
#              it is the system ports collection directory.
#  INSTALL_PORTS - whether to install ports mt32emu-qt and mt32emu-smf2wav in the system as they
#                  get created. Note, the port libmt32emu is installed anyway.
#  libmt32emu_VERSION - the version of the library mt32emu to generate a port for.
#  mt32emu_qt_VERSION - the version of the application mt32emu-qt to generate a port for.
#  mt32emu_smf2wav_VERSION - the version of the application mt32emu-smf2wav to generate a port for.
# If the variables xxx_VERSION not set, they default to the versions set in the corresponding CMake
# lists files in the source tree.

set(MASTER_SITES "https://github.com/munt/munt/archive/")
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

if(NOT DEFINED PORTS_DIR)
  set(PORTS_DIR "/usr/ports")
endif()

if(NOT DEFINED INSTALL_PORTS)
  set(INSTALL_PORTS FALSE)
endif()

macro(CHECK_MAKE_ERROR ACTION_NAME)
  if(NOT MAKE_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to ${ACTION_NAME} for port ${project_dir} with error ${MAKE_EXIT_CODE}")
  endif()
endmacro()

function(generate_port project_dir port_name should_install)
  set(PORT_DIR "${PORTS_DIR}/audio/${port_name}")
  string(MAKE_C_IDENTIFIER ${port_name} PROJECT_ID)

  set(VERSION_OVERRIDE ${${PROJECT_ID}_VERSION})
  include("${SOURCE_DIR}/${project_dir}/cmake/project_data.cmake")
  if(VERSION_OVERRIDE)
    set(${PROJECT_ID}_VERSION ${VERSION_OVERRIDE})
  endif()
  string(MAKE_C_IDENTIFIER ${PROJECT_ID}_${${PROJECT_ID}_VERSION} ${PROJECT_ID}_DISTFILE_BASENAME)

  configure_file("${SOURCE_DIR}/FreeBSD/${project_dir}/Makefile.in" "${PORT_DIR}/Makefile" @ONLY)
  configure_file("${SOURCE_DIR}/${project_dir}/info.txt" "${PORT_DIR}/pkg-descr" COPYONLY)
  file(APPEND "${PORT_DIR}/pkg-descr" "\nWWW: ${${PROJECT_ID}_URL}\n")

  message(STATUS "Generating checksum and building for ${project_dir}")
  execute_process(
    COMMAND make makesum stage
    WORKING_DIRECTORY "${PORT_DIR}"
    RESULT_VARIABLE MAKE_EXIT_CODE
  )
  CHECK_MAKE_ERROR("generate checksum file")

  message(STATUS "Generating plist for ${project_dir}")
  execute_process(
    COMMAND make makeplist
    COMMAND grep -v "/you/have/to/check/what/makeplist/gives/you" -
    WORKING_DIRECTORY "${PORT_DIR}"
    RESULT_VARIABLE MAKE_EXIT_CODE
    OUTPUT_FILE pkg-plist
  )
  CHECK_MAKE_ERROR("generate plist file")

  if(should_install)
    message(STATUS "Installing port for ${project_dir}")
    execute_process(
      COMMAND make restage reinstall clean
      WORKING_DIRECTORY "${PORT_DIR}"
      RESULT_VARIABLE MAKE_EXIT_CODE
    )
    CHECK_MAKE_ERROR("install")
  else()
    message(STATUS "Cleaning port for ${project_dir}")
    execute_process(
      COMMAND make clean
      WORKING_DIRECTORY "${PORT_DIR}"
      RESULT_VARIABLE MAKE_EXIT_CODE
    )
    CHECK_MAKE_ERROR("clean")
  endif()
endfunction()

generate_port(mt32emu libmt32emu TRUE)
generate_port(mt32emu_qt mt32emu-qt ${INSTALL_PORTS})
generate_port(mt32emu_smf2wav mt32emu-smf2wav ${INSTALL_PORTS})
