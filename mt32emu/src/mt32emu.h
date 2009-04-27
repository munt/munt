/* Copyright (C) 2003-2009 Dean Beeler, Jerome Fisher
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

// Debugging
// Show the instruments played
#define MT32EMU_MONITOR_INSTRUMENTS 0
// Shows number of partials MT-32 is playing, and on which parts
#define MT32EMU_MONITOR_PARTIALS 0

#define MT32EMU_USE_EXTINT 0

// Configuration
// The maximum number of partials playing simultaneously
#define MT32EMU_MAX_PARTIALS 32
// The maximum number of notes playing simultaneously per part.
// No point making it more than MT32EMU_MAX_PARTIALS, since each note needs at least one partial.
#define MT32EMU_MAX_POLY 32

#if (defined (_MSC_VER) && defined(_M_IX86))
#define MT32EMU_HAVE_X86
#elif  defined(__GNUC__)
// FIXME: Our use of inline assembler is broken in gcc 4, at least for some MMX code.
// Temporarily enabling inline assembly only for gcc 3.
#if __GNUC__ == 3 && defined(__i386__)
#define MT32EMU_HAVE_X86
#endif
#endif

#ifdef MT32EMU_HAVE_X86
#define MT32EMU_USE_MMX 1
#else
#define MT32EMU_USE_MMX 0
#endif

#include "freeverb/revmodel.h"

#include "structures.h"
#include "i386.h"
#include "file.h"
#include "tables.h"
#include "poly.h"
#include "tva.h"
#include "tvp.h"
#include "tvf.h"
#include "partial.h"
#include "partialManager.h"
#include "part.h"
#include "synth.h"

#endif
