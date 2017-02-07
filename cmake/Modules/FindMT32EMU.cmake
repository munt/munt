# - Try to find mt32emu
# Once done this will define
#  MT32EMU_FOUND - System has mt32emu
#  MT32EMU_INCLUDE_DIRS - The location of the mt32emu include directory
#  MT32EMU_LIBRARIES - The location of the mt32emu library

find_path(MT32EMU_INCLUDE_DIR mt32emu/mt32emu.h)

find_library(MT32EMU_LIBRARY mt32emu)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set MT32EMU_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(MT32EMU DEFAULT_MSG MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR)

set(MT32EMU_LIBRARIES ${MT32EMU_LIBRARY} ${MT32EMU_EXT_LIBS})
set(MT32EMU_INCLUDE_DIRS ${MT32EMU_INCLUDE_DIR})

mark_as_advanced(MT32EMU_EXT_LIBS MT32EMU_LIBRARY MT32EMU_INCLUDE_DIR)
