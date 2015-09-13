/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2015 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_MT32EMU_H
#define MT32EMU_MT32EMU_H

/* API Configuration */

/* 0: Use full-featured C++ API. Well suitable when the library is to be linked statically.
 *    When the library is shared, ABI compatibility may be an issue. Therefore, it should
 *    only be used within a project comprising of several modules to share the library code.
 * 1: Use C-compatible API. Make the library looks as a regular C library with well-defined ABI.
 *    This is also crucial when the library is to be linked with objects compiled in a different
 *    language, either statically or dynamically.
 * 2: Use plugin-like API via C++ abstract classes. This is mainly intended for a shared library
 *    being dynamically loaded in run-type. To get access to all the library services, a client
 *    application only needs to bind with a single factory function.
 */
#ifndef MT32EMU_API_TYPE
#define MT32EMU_API_TYPE 0
#endif

#ifndef MT32EMU_EXPORTS_TYPE
#define MT32EMU_EXPORTS_TYPE MT32EMU_API_TYPE
#endif

#include "globals.h"

#if defined(__cplusplus) && MT32EMU_API_TYPE != 1

#if MT32EMU_API_TYPE == 0

#include "Types.h"
#include "File.h"
#include "FileStream.h"
#include "ROMInfo.h"
#include "Synth.h"
#include "MidiStreamParser.h"

#else /* MT32EMU_API_TYPE == 0 */

#include "c_interface/cpp_interface.h"

#endif /* MT32EMU_API_TYPE == 0 */

#else /* #if defined(__cplusplus) && MT32EMU_API_TYPE != 1 */

#include "c_interface/c_interface.h"

#endif /* #if defined(__cplusplus) && MT32EMU_API_TYPE != 1 */

#endif /* #ifndef MT32EMU_MT32EMU_H */
