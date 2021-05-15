cmake_minimum_required(VERSION 2.8.12)
project(mt32emu_win32drv_setup)

if(NOT CMAKE_CL_64)
  add_subdirectory(drvsetup)
endif()
add_subdirectory(infinstall)
