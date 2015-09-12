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

#include "globals.h"

#if defined(__cplusplus) && !MT32EMU_C_INTERFACE

#ifdef MT32EMU_CPP_PLUGIN_INTERFACE

#include "c_interface/cpp_interface.h"

#else /* #ifdef MT32EMU_CPP_PLUGIN_INTERFACE */

#include "Types.h"
#include "File.h"
#include "FileStream.h"
#include "ROMInfo.h"
#include "Synth.h"
#include "MidiStreamParser.h"

#endif /* #ifdef MT32EMU_CPP_PLUGIN_INTERFACE */

#else /* #if defined(__cplusplus) && !MT32EMU_C_INTERFACE */

#include "c_interface/c_interface.h"

#endif /* #if defined(__cplusplus) && !MT32EMU_C_INTERFACE */

#endif /* #ifndef MT32EMU_MT32EMU_H */
