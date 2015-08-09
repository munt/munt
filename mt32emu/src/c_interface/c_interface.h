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

// Prepares the emulation context to receive MIDI messages and produce output audio data using aforehand added set of ROMs.
// partial_count sets the maximum number of partials playing simultaneously for this session (optional).
// analog_output_mode sets the mode for emulation of analogue circuitry of the hardware units (optional).
// If either partial_count and/or analog_output_mode arguments is set to NULL, the default value will be used.
// Returns MT32EMU_RC_OK upon success.
enum mt32emu_return_code mt32emu_open_synth(mt32emu_const_context context, const unsigned int *partial_count, const enum mt32emu_analog_output_mode *analog_output_mode);

// Closes the emulation context freeing allocated resources. Added ROMs remain unaffected and ready for reuse.
void mt32emu_close_synth(mt32emu_const_context context);

// Returns output sample rate used in emulation of stereo analog circuitry of hardware units for particular analog_output_mode.
// See comment for mt32emu_analog_output_mode.
unsigned int mt32emu_get_stereo_output_samplerate(const enum mt32emu_analog_output_mode analog_output_mode);

// Returns actual output sample rate used in emulation of stereo analog circuitry of hardware units.
// See comment for mt32emu_analog_output_mode.
unsigned int mt32emu_get_actual_stereo_output_samplerate(mt32emu_const_context context);

// All the enqueued events are processed by the synth immediately.
void mt32emu_flush_midi_queue();

// Sets size of the internal MIDI event queue. The queue size is set to the minimum power of 2 that is greater or equal to the size specified.
// The queue is flushed before reallocation.
// Returns the actual queue size being used.
mt32emu_bit32u mt32emu_setMIDIEventQueueSize(mt32emu_bit32u);

// Enqueues a MIDI event for subsequent playback.
// The MIDI event will be processed not before the specified timestamp.
// The timestamp is measured as the global rendered sample count since the synth was created (at the native sample rate 32000 Hz).
// The minimum delay involves emulation of the delay introduced while the event is transferred via MIDI interface
// and emulation of the MCU busy-loop while it frees partials for use by a new Poly.
// Calls from multiple threads must be synchronised, although, no synchronisation is required with the rendering thread.
// The methods return false if the MIDI event queue is full and the message cannot be enqueued.

// Enqueues a single short MIDI message. The message must contain a status byte.
enum mt32emu_return_code mt32emu_playMsg(mt32emu_bit32u msg, mt32emu_bit32u timestamp);
// Enqueues a single well formed System Exclusive MIDI message.
enum mt32emu_return_code mt32emu_playSysex(const mt32emu_bit8u *sysex, mt32emu_bit32u len, mt32emu_bit32u timestamp);

// Overloaded methods for the MIDI events to be processed ASAP.
enum mt32emu_return_code mt32emu_playMsg(mt32emu_bit32u msg);
enum mt32emu_return_code mt32emu_playSysex(const mt32emu_bit8u *sysex, mt32emu_bit32u len);

// WARNING:
// The methods below don't ensure minimum 1-sample delay between sequential MIDI events,
// and a sequence of NoteOn and immediately succeeding NoteOff messages is always silent.
// A thread that invokes these methods must be explicitly synchronised with the thread performing sample rendering.

// Sends a short MIDI message to the synth for immediate playback. The message must contain a status byte.
void mt32emu_playMsgNow(mt32emu_bit32u msg);
void mt32emu_playMsgOnPart(unsigned char part, unsigned char code, unsigned char note, unsigned char velocity);

// Sends a string of Sysex commands to the MT-32 for immediate interpretation
// The length is in bytes
void mt32emu_playSysexNow(const mt32emu_bit8u *sysex, mt32emu_bit32u len);
void mt32emu_playSysexWithoutFraming(const mt32emu_bit8u *sysex, mt32emu_bit32u len);
void playSysexWithoutHeader(unsigned char device, unsigned char command, const mt32emu_bit8u *sysex, mt32emu_bit32u len);
void writeSysex(unsigned char channel, const mt32emu_bit8u *sysex, mt32emu_bit32u len);

void mt32emu_setReverbEnabled(const enum mt32emu_boolean reverbEnabled);
enum mt32emu_boolean mt32emu_isReverbEnabled();
// Sets override reverb mode. In this mode, emulation ignores sysexes (or the related part of them) which control the reverb parameters.
// This mode is in effect until it is turned off. When the synth is re-opened, the override mode is unchanged but the state
// of the reverb model is reset to default.
void mt32emu_setReverbOverridden(const enum mt32emu_boolean reverbOverridden);
enum mt32emu_boolean mt32emu_isReverbOverridden();
// Forces reverb model compatibility mode. By default, the compatibility mode corresponds to the used control ROM version.
// Invoking this method with the argument set to true forces emulation of old MT-32 reverb circuit.
// When the argument is false, emulation of the reverb circuit used in new generation of MT-32 compatible modules is enforced
// (these include CM-32L and LAPC-I).
void mt32emu_setReverbCompatibilityMode(const enum mt32emu_boolean mt32CompatibleMode);
enum mt32emu_boolean mt32emu_isMT32ReverbCompatibilityMode();
enum mt32emu_boolean mt32emu_isDefaultReverbMT32Compatible();

void mt32emu_setDACInputMode(const enum mt32emu_dac_input_mode mode);
enum mt32emu_dac_input_mode mt32emu_getDACInputMode();

void mt32emu_setMIDIDelayMode(const enum mt32emu_midi_delay_mode mode);
enum mt32emu_midi_delay_mode mt32emu_getMIDIDelayMode();

// Sets output gain factor for synth output channels. Applied to all output samples and unrelated with the synth's Master volume,
// it rather corresponds to the gain of the output analog circuitry of the hardware units. However, together with setReverbOutputGain()
// it offers to the user a capability to control the gain of reverb and non-reverb output channels independently.
// Ignored in DACInputMode_PURE
void mt32emu_setOutputGain(float);
float mt32emu_getOutputGain();

// Sets output gain factor for the reverb wet output channels. It rather corresponds to the gain of the output
// analog circuitry of the hardware units. However, together with setOutputGain() it offers to the user a capability
// to control the gain of reverb and non-reverb output channels independently.
//
// Note: We're currently emulate CM-32L/CM-64 reverb quite accurately and the reverb output level closely
// corresponds to the level of digital capture. Although, according to the CM-64 PCB schematic,
// there is a difference in the reverb analogue circuit, and the resulting output gain is 0.68
// of that for LA32 analogue output. This factor is applied to the reverb output gain.
// Ignored in DACInputMode_PURE
void mt32emu_setReverbOutputGain(float);
float mt32emu_getReverbOutputGain();

void mt32emu_setReversedStereoEnabled(const enum mt32emu_boolean enabled);
enum mt32emu_boolean mt32emu_isReversedStereoEnabled();

// Renders samples to the specified output stream as if they were sampled at the analog stereo output.
// When AnalogOutputMode is set to ACCURATE, the output signal is upsampled to 48 kHz in order
// to retain emulation accuracy in whole audible frequency spectra. Otherwise, native digital signal sample rate is retained.
// getStereoOutputSampleRate() can be used to query actual sample rate of the output signal.
// The length is in frames, not bytes (in 16-bit stereo, one frame is 4 bytes).
void mt32emu_render(mt32emu_sample *stream, mt32emu_bit32u len);

// Renders samples to the specified output streams as if they appeared at the DAC entrance.
// No further processing performed in analog circuitry emulation is applied to the signal.
// NULL may be specified in place of any or all of the stream buffers.
// The length is in samples, not bytes.
void mt32emu_renderStreams(mt32emu_sample *nonReverbLeft, mt32emu_sample *nonReverbRight, mt32emu_sample *reverbDryLeft, mt32emu_sample *reverbDryRight, mt32emu_sample *reverbWetLeft, mt32emu_sample *reverbWetRight, mt32emu_bit32u len);

// Returns true when there is at least one active partial, otherwise false.
enum mt32emu_boolean mt32emu_hasActivePartials();

// Returns true if hasActivePartials() returns true, or reverb is (somewhat unreliably) detected as being active.
enum mt32emu_boolean mt32emu_isActive();

// Returns the maximum number of partials playing simultaneously.
unsigned int mt32emu_getPartialCount();

// Fills in current states of all the parts into the array provided. The array must have at least 9 entries to fit values for all the parts.
// If the value returned for a part is true, there is at least one active non-releasing partial playing on this part.
// This info is useful in emulating behaviour of LCD display of the hardware units.
void mt32emu_getPartStates(enum mt32emu_boolean *partStates);

// Fills in current states of all the partials into the array provided. The array must be large enough to accommodate states of all the partials.
void mt32emu_getPartialStates(enum mt32emu_partial_state *partialStates);

// Fills in information about currently playing notes on the specified part into the arrays provided. The arrays must be large enough
// to accommodate data for all the playing notes. The maximum number of simultaneously playing notes cannot exceed the number of partials.
// Argument partNumber should be 0..7 for Part 1..8, or 8 for Rhythm.
// Returns the number of currently playing notes on the specified part.
unsigned int mt32emu_getPlayingNotes(unsigned int partNumber, mt32emu_bit8u *keys, mt32emu_bit8u *velocities);

// Returns name of the patch set on the specified part.
// Argument partNumber should be 0..7 for Part 1..8, or 8 for Rhythm.
const char *mt32emu_getPatchName(unsigned int partNumber);

void mt32emu_readMemory(mt32emu_bit32u addr, mt32emu_bit32u len, mt32emu_bit8u *data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef MT32EMU_C_INTERFACE_H
