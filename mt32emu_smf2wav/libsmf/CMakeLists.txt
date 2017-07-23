project(libsmf C)

set(libsmf_VERSION_MAJOR 1)
set(libsmf_VERSION_MINOR 2)

find_package(GLIB2 REQUIRED)
include_directories(${GLIB2_INCLUDE_DIRS})

add_definitions(-DG_LOG_DOMAIN="libsmf")

configure_file(
  src/config.h.in
  "${PROJECT_BINARY_DIR}/config.h"
)
include_directories("${PROJECT_BINARY_DIR}")

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_CRT_SECURE_NO_WARNINGS=1)
endif()

add_library(smf
  src/smf.c
  src/smf_decode.c
  src/smf_load.c
  src/smf_save.c
  src/smf_tempo.c
)

target_link_libraries(smf ${GLIB2_LIBRARIES})
