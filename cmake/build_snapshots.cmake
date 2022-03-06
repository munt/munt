if(NOT munt_GIT_HASH)
  find_package(Git REQUIRED)
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h --abbrev=10
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE munt_GIT_EXIT_CODE
    OUTPUT_VARIABLE munt_GIT_HASH
  )
  if(NOT munt_GIT_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to run ${GIT_EXECUTABLE} for retrieving hash of current HEAD with error ${munt_GIT_EXIT_CODE}")
  endif()
  unset(munt_GIT_EXIT_CODE)
  set(munt_GIT_HASH ${munt_GIT_HASH} PARENT_SCOPE)
endif()

string(STRIP "${munt_GIT_HASH}" munt_GIT_HASH)
string(MAKE_C_IDENTIFIER ${PROJECT_NAME} munt_PROJECT_ID)
if(munt_PROJECT_ID STREQUAL "libmt32emu")
  set(libmt32emu_SNAPSHOT_VERSION "${libmt32emu_VERSION_MAJOR}.${libmt32emu_VERSION_MINOR}-git${munt_GIT_HASH}")
  set(libmt32emu_SNAPSHOT_VERSION "${libmt32emu_SNAPSHOT_VERSION}" PARENT_SCOPE)
  message(STATUS "Building snapshot libmt32emu-${libmt32emu_SNAPSHOT_VERSION}")
else()
  set(${munt_PROJECT_ID}_VERSION "${${munt_PROJECT_ID}_VERSION_MAJOR}.${${munt_PROJECT_ID}_VERSION_MINOR}-git${munt_GIT_HASH}")
  message(STATUS "Building snapshot ${munt_PROJECT_ID}-${${munt_PROJECT_ID}_VERSION}")
endif()

unset(munt_PROJECT_ID)
