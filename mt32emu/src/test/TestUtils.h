/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2024 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_TESTUTILS_H
#define MT32EMU_TESTUTILS_H

#include "../Types.h"

namespace MT32Emu {

class Synth;

namespace Test {

class ROMSet;

void openSynth(Synth &synth, const ROMSet &romSet);
void sendMasterVolumeSysex(Synth &synth, Bit8u volume);
void sendAllNotesOff(Synth &synth, Bit8u channel);
void sendNoteOn(Synth &synth, Bit8u channel, Bit8u note, Bit8u velocity);
// Configures the patch & timbre temp area with a timbre that outputs a pure sine wave with a period of exactly 256 samples
// at the maximum amplitude in the right channel.
void sendSineWaveSysex(Synth &synth, Bit8u channel);

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_TESTUTILS_H
