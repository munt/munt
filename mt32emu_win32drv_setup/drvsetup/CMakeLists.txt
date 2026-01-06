cmake_minimum_required(VERSION 2.8.12...3.27)

project(mt32emu_drvsetup CXX)

if(NOT MSVC)
  # Building with MinGW is not currently supported.
  message(FATAL_ERROR "Microsoft (R) C++ compiler is currently required")
endif()
if(CMAKE_CL_64)
  message(FATAL_ERROR "Building 64-bit binary not supported; 32-bit binary will work on both platforms")
endif()

set(${PROJECT_NAME}_SOURCES drvsetup.cpp)

if(MSVC_IDE)
  add_compile_options(-Yu)
  set_source_files_properties(stdafx.cpp
    PROPERTIES COMPILE_FLAGS -Yc
  )
  list(APPEND ${PROJECT_NAME}_SOURCES stdafx.cpp)
endif()

add_definitions(
  -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
  -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1
  -D_CRT_SECURE_NO_WARNINGS
)
add_executable(drvsetup ${${PROJECT_NAME}_SOURCES})
target_link_libraries(drvsetup SetupAPI.Lib)
set_target_properties(drvsetup
  PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /MANIFESTUAC:level='requireAdministrator' /SUBSYSTEM:WINDOWS"
)
