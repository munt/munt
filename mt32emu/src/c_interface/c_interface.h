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

#ifndef MT32EMU_C_INTERFACE_H
#define MT32EMU_C_INTERFACE_H

#include "c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sample rate to use in mixing. With the progress of development, we've found way too many thing dependent.
// In order to achieve further advance in emulation accuracy, sample rate made fixed throughout the emulator,
// except the emulation of analogue path.
// The output from the synth is supposed to be resampled externally in order to convert to the desired sample rate.
extern const unsigned int MT32EMU_SAMPLE_RATE;

// The default value for the maximum number of partials playing simultaneously.
extern const unsigned int MT32EMU_DEFAULT_MAX_PARTIALS;

// The higher this number, the more memory will be used, but the more samples can be processed in one run -
// various parts of sample generation can be processed more efficiently in a single run.
// A run's maximum length is that given to Synth::render(), so giving a value here higher than render() is ever
// called with will give no gain (but simply waste the memory).
// Note that this value does *not* in any way impose limitations on the length given to render(), and has no effect
// on the generated audio.
// This value must be >= 1.
extern const unsigned int MT32EMU_MAX_SAMPLES_PER_RUN;

// Initialises a new emulation context and installs custom report handler if non-NULL.
mt32emu_context mt32emu_create_synth(const struct mt32emu_report_handler_o *report_handler);

// Closes and destroys emulation context.
void mt32emu_free_synth(mt32emu_context context);

// Adds new ROM identified by its SHA1 digest to the emulation context replacing previously added ROM of the same type if any.
// Argument sha1_digest can be NULL, in this case the digest will be computed using the actual ROM data.
// If sha1_digest is set to non-NULL, it is assumed correct and will not be recomputed.
// This function doesn't immediately change the state of already opened synth. Newly added ROM will take effect upon next call of mt32emu_open_synth().
// Returns positive value upon success.
enum mt32emu_return_code mt32emu_add_rom_data(mt32emu_context context, const mt32emu_bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest);

// Loads a ROM file, identify it by SHA1 digest, and adds it to the emulation context replacing previously added ROM of the same type if any.
// This function doesn't immediately change the state of already opened synth. Newly added ROM will take effect upon next call of mt32emu_open_synth().
// Returns positive value upon success.
enum mt32emu_return_code mt32emu_add_rom_file(mt32emu_context context, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // #ifndef MT32EMU_C_INTERFACE_H
