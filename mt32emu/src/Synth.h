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

#ifndef MT32EMU_SYNTH_H
#define MT32EMU_SYNTH_H

#include <cstdarg>
#include <cstddef>
#include <cstring>

#include "globals.h"
#include "Types.h"
#include "Enumerations.h"

namespace MT32Emu {

class Analog;
class BReverbModel;
class MemoryRegion;
class MidiEventQueue;
class Part;
class Poly;
class Partial;
class PartialManager;
class ROMImage;

class PatchTempMemoryRegion;
class RhythmTempMemoryRegion;
class TimbreTempMemoryRegion;
class PatchesMemoryRegion;
class TimbresMemoryRegion;
class SystemMemoryRegion;
class DisplayMemoryRegion;
class ResetMemoryRegion;

struct ControlROMFeatureSet;
struct ControlROMMap;
struct PCMWaveEntry;
struct MemParams;

const Bit8u SYSEX_MANUFACTURER_ROLAND = 0x41;

const Bit8u SYSEX_MDL_MT32 = 0x16;
const Bit8u SYSEX_MDL_D50 = 0x14;

const Bit8u SYSEX_CMD_RQ1 = 0x11; // Request data #1
const Bit8u SYSEX_CMD_DT1 = 0x12; // Data set 1
const Bit8u SYSEX_CMD_WSD = 0x40; // Want to send data
const Bit8u SYSEX_CMD_RQD = 0x41; // Request data
const Bit8u SYSEX_CMD_DAT = 0x42; // Data set
const Bit8u SYSEX_CMD_ACK = 0x43; // Acknowledge
const Bit8u SYSEX_CMD_EOD = 0x45; // End of data
const Bit8u SYSEX_CMD_ERR = 0x4E; // Communications error
const Bit8u SYSEX_CMD_RJC = 0x4F; // Rejection

const unsigned int CONTROL_ROM_SIZE = 64 * 1024;

class MT32EMU_EXPORT ReportHandler {
friend class Synth;

public:
	virtual ~ReportHandler() {}

protected:

	// Callback for debug messages, in vprintf() format
	virtual void printDebug(const char *fmt, va_list list);

	// Callbacks for reporting various errors and information
	virtual void onErrorControlROM() {}
	virtual void onErrorPCMROM() {}
	virtual void showLCDMessage(const char *message);
	virtual void onMIDIMessagePlayed() {}
	virtual void onDeviceReset() {}
	virtual void onDeviceReconfig() {}
	virtual void onNewReverbMode(Bit8u /* mode */) {}
	virtual void onNewReverbTime(Bit8u /* time */) {}
	virtual void onNewReverbLevel(Bit8u /* level */) {}
	virtual void onPolyStateChanged(int /* partNum */) {}
	virtual void onProgramChanged(int /* partNum */, const char * /* soundGroupName */, const char * /* patchName */) {}
};

class MT32EMU_EXPORT Synth {
friend class Part;
friend class RhythmPart;
friend class Poly;
friend class Partial;
friend class PartialManager;
friend class TVA;
friend class TVP;
private:
	PatchTempMemoryRegion *patchTempMemoryRegion;
	RhythmTempMemoryRegion *rhythmTempMemoryRegion;
	TimbreTempMemoryRegion *timbreTempMemoryRegion;
	PatchesMemoryRegion *patchesMemoryRegion;
	TimbresMemoryRegion *timbresMemoryRegion;
	SystemMemoryRegion *systemMemoryRegion;
	DisplayMemoryRegion *displayMemoryRegion;
	ResetMemoryRegion *resetMemoryRegion;

	Bit8u *paddedTimbreMaxTable;

	bool isEnabled;

	PCMWaveEntry *pcmWaves; // Array

	const ControlROMFeatureSet *controlROMFeatures;
	const ControlROMMap *controlROMMap;
	Bit8u controlROMData[CONTROL_ROM_SIZE];
	Bit16s *pcmROMData;
	size_t pcmROMSize; // This is in 16-bit samples, therefore half the number of bytes in the ROM

	Bit8u soundGroupIx[128]; // For each standard timbre
	const char (*soundGroupNames)[9]; // Array

	unsigned int partialCount;
	Bit8s chantable[32]; // FIXME: Need explanation why 32 is set, obviously it should be 16

	MidiEventQueue *midiQueue;
	volatile Bit32u lastReceivedMIDIEventTimestamp;
	volatile Bit32u renderedSampleCount;

	MemParams &mt32ram, &mt32default;

	BReverbModel *reverbModels[4];
	BReverbModel *reverbModel;
	bool reverbOverridden;

	MIDIDelayMode midiDelayMode;
	DACInputMode dacInputMode;

	float outputGain;
	float reverbOutputGain;

	bool reversedStereoEnabled;

	bool opened;

	bool isDefaultReportHandler;
	ReportHandler *reportHandler;

	PartialManager *partialManager;
	Part *parts[9];

	// When a partial needs to be aborted to free it up for use by a new Poly,
	// the controller will busy-loop waiting for the sound to finish.
	// We emulate this by delaying new MIDI events processing until abortion finishes.
	Poly *abortingPoly;

	Analog *analog;

	Bit32u addMIDIInterfaceDelay(Bit32u len, Bit32u timestamp);

	void produceLA32Output(Sample *buffer, Bit32u len);
	void convertSamplesToOutput(Sample *buffer, Bit32u len);
	bool isAbortingPoly() const;
	void doRenderStreams(Sample *nonReverbLeft, Sample *nonReverbRight, Sample *reverbDryLeft, Sample *reverbDryRight, Sample *reverbWetLeft, Sample *reverbWetRight, Bit32u len);

	void readSysex(unsigned char channel, const Bit8u *sysex, Bit32u len) const;
	void initMemoryRegions();
	void deleteMemoryRegions();
	MemoryRegion *findMemoryRegion(Bit32u addr);
	void writeMemoryRegion(const MemoryRegion *region, Bit32u addr, Bit32u len, const Bit8u *data);
	void readMemoryRegion(const MemoryRegion *region, Bit32u addr, Bit32u len, Bit8u *data);

	bool loadControlROM(const ROMImage &controlROMImage);
	bool loadPCMROM(const ROMImage &pcmROMImage);

	bool initPCMList(Bit16u mapAddress, Bit16u count);
	bool initTimbres(Bit16u mapAddress, Bit16u offset, int timbreCount, int startTimbre, bool compressed);
	bool initCompressedTimbre(int drumNum, const Bit8u *mem, unsigned int memLen);
	void initSoundGroups(char newSoundGroupNames[][9]);

	void refreshSystemMasterTune();
	void refreshSystemReverbParameters();
	void refreshSystemReserveSettings();
	void refreshSystemChanAssign(unsigned int firstPart, unsigned int lastPart);
	void refreshSystemMasterVol();
	void refreshSystem();
	void reset();

	void printPartialUsage(unsigned long sampleOffset = 0);

	void polyStateChanged(int partNum);
	void newTimbreSet(int partNum, Bit8u timbreGroup, Bit8u timbreNumber, const char patchName[]);
	void printDebug(const char *fmt, ...);

	// partNum should be 0..7 for Part 1..8, or 8 for Rhythm
	const Part *getPart(unsigned int partNum) const;

public:
	static inline Sample clipSampleEx(SampleEx sampleEx) {
#if MT32EMU_USE_FLOAT_SAMPLES
		return sampleEx;
#else
		// Clamp values above 32767 to 32767, and values below -32768 to -32768
		// FIXME: Do we really need this stuff? I think these branches are very well predicted. Instead, this introduces a chain.
		// The version below is actually a bit faster on my system...
		//return ((sampleEx + 0x8000) & ~0xFFFF) ? (sampleEx >> 31) ^ 0x7FFF : (Sample)sampleEx;
		return ((-0x8000 <= sampleEx) && (sampleEx <= 0x7FFF)) ? (Sample)sampleEx : (sampleEx >> 31) ^ 0x7FFF;
#endif
	}

	static inline void muteSampleBuffer(Sample *buffer, Bit32u len) {
		if (buffer == NULL) return;

#if MT32EMU_USE_FLOAT_SAMPLES
		// FIXME: Use memset() where compatibility is guaranteed (if this turns out to be a win)
		while (len--) {
			*(buffer++) = 0.0f;
		}
#else
		memset(buffer, 0, len * sizeof(Sample));
#endif
	}

	static Bit32u getShortMessageLength(Bit32u msg);
	static Bit8u calcSysexChecksum(const Bit8u *data, const Bit32u len, const Bit8u initChecksum = 0);

	// Returns output sample rate used in emulation of stereo analog circuitry of hardware units.
	// See comment for AnalogOutputMode.
	static unsigned int getStereoOutputSampleRate(AnalogOutputMode analogOutputMode);

	// Optionally sets callbacks for reporting various errors, information and debug messages
	Synth(ReportHandler *useReportHandler = NULL);
	~Synth();

	// Used to initialise the MT-32. Must be called before any other function.
	// Returns true if initialization was sucessful, otherwise returns false.
	// controlROMImage and pcmROMImage represent Control and PCM ROM images for use by synth.
	// usePartialCount sets the maximum number of partials playing simultaneously for this session (optional).
	// analogOutputMode sets the mode for emulation of analogue circuitry of the hardware units (optional).
	bool open(const ROMImage &controlROMImage, const ROMImage &pcmROMImage, unsigned int usePartialCount = DEFAULT_MAX_PARTIALS, AnalogOutputMode analogOutputMode = AnalogOutputMode_COARSE);

	// Overloaded method which opens the synth with default partial count.
	bool open(const ROMImage &controlROMImage, const ROMImage &pcmROMImage, AnalogOutputMode analogOutputMode);

	// Closes the MT-32 and deallocates any memory used by the synthesizer
	void close(bool forced = false);

	// Returns true if the synth is in completely initialized state, otherwise returns false.
	bool isOpen() const;

	// All the enqueued events are processed by the synth immediately.
	void flushMIDIQueue();

	// Sets size of the internal MIDI event queue. The queue size is set to the minimum power of 2 that is greater or equal to the size specified.
	// The queue is flushed before reallocation.
	// Returns the actual queue size being used.
	Bit32u setMIDIEventQueueSize(Bit32u);

	// Enqueues a MIDI event for subsequent playback.
	// The MIDI event will be processed not before the specified timestamp.
	// The timestamp is measured as the global rendered sample count since the synth was created (at the native sample rate 32000 Hz).
	// The minimum delay involves emulation of the delay introduced while the event is transferred via MIDI interface
	// and emulation of the MCU busy-loop while it frees partials for use by a new Poly.
	// Calls from multiple threads must be synchronised, although, no synchronisation is required with the rendering thread.
	// The methods return false if the MIDI event queue is full and the message cannot be enqueued.

	// Enqueues a single short MIDI message to play at specified time. The message must contain a status byte.
	bool playMsg(Bit32u msg, Bit32u timestamp);
	// Enqueues a single well formed System Exclusive MIDI message to play at specified time.
	bool playSysex(const Bit8u *sysex, Bit32u len, Bit32u timestamp);

	// Enqueues a single short MIDI message to be processed ASAP. The message must contain a status byte.
	bool playMsg(Bit32u msg);
	// Enqueues a single well formed System Exclusive MIDI message to be processed ASAP.
	bool playSysex(const Bit8u *sysex, Bit32u len);

	// WARNING:
	// The methods below don't ensure minimum 1-sample delay between sequential MIDI events,
	// and a sequence of NoteOn and immediately succeeding NoteOff messages is always silent.
	// A thread that invokes these methods must be explicitly synchronised with the thread performing sample rendering.

	// Sends a short MIDI message to the synth for immediate playback. The message must contain a status byte.
	// See the WARNING above.
	void playMsgNow(Bit32u msg);
	// Sends unpacked short MIDI message to the synth for immediate playback. The message must contain a status byte.
	// See the WARNING above.
	void playMsgOnPart(unsigned char part, unsigned char code, unsigned char note, unsigned char velocity);

	// Sends a single well formed System Exclusive MIDI message for immediate processing. The length is in bytes.
	// See the WARNING above.
	void playSysexNow(const Bit8u *sysex, Bit32u len);
	// Sends inner body of a System Exclusive MIDI message for direct processing. The length is in bytes.
	// See the WARNING above.
	void playSysexWithoutFraming(const Bit8u *sysex, Bit32u len);
	// Sends inner body of a System Exclusive MIDI message for direct processing. The length is in bytes.
	// See the WARNING above.
	void playSysexWithoutHeader(unsigned char device, unsigned char command, const Bit8u *sysex, Bit32u len);
	// Sends inner body of a System Exclusive MIDI message for direct processing. The length is in bytes.
	// See the WARNING above.
	void writeSysex(unsigned char channel, const Bit8u *sysex, Bit32u len);

	// Allows to disable wet reverb output altogether.
	void setReverbEnabled(bool reverbEnabled);
	// Returns whether wet reverb output is enabled.
	bool isReverbEnabled() const;
	// Sets override reverb mode. In this mode, emulation ignores sysexes (or the related part of them) which control the reverb parameters.
	// This mode is in effect until it is turned off. When the synth is re-opened, the override mode is unchanged but the state
	// of the reverb model is reset to default.
	void setReverbOverridden(bool reverbOverridden);
	// Returns whether reverb settings are overridden.
	bool isReverbOverridden() const;
	// Forces reverb model compatibility mode. By default, the compatibility mode corresponds to the used control ROM version.
	// Invoking this method with the argument set to true forces emulation of old MT-32 reverb circuit.
	// When the argument is false, emulation of the reverb circuit used in new generation of MT-32 compatible modules is enforced
	// (these include CM-32L and LAPC-I).
	void setReverbCompatibilityMode(bool mt32CompatibleMode);
	// Returns whether reverb is in old MT-32 compatibility mode.
	bool isMT32ReverbCompatibilityMode() const;
	// Returns whether default reverb compatibility mode is the old MT-32 compatibility mode.
	bool isDefaultReverbMT32Compatible() const;
	// Sets new DAC input mode. See DACInputMode for details.
	void setDACInputMode(DACInputMode mode);
	// Returns current DAC input mode. See DACInputMode for details.
	DACInputMode getDACInputMode() const;
	// Sets new MIDI delay mode. See MIDIDelayMode for details.
	void setMIDIDelayMode(MIDIDelayMode mode);
	// Returns current MIDI delay mode. See MIDIDelayMode for details.
	MIDIDelayMode getMIDIDelayMode() const;

	// Sets output gain factor for synth output channels. Applied to all output samples and unrelated with the synth's Master volume,
	// it rather corresponds to the gain of the output analog circuitry of the hardware units. However, together with setReverbOutputGain()
	// it offers to the user a capability to control the gain of reverb and non-reverb output channels independently.
	// Ignored in DACInputMode_PURE
	void setOutputGain(float gain);
	// Returns current output gain factor for synth output channels.
	float getOutputGain() const;

	// Sets output gain factor for the reverb wet output channels. It rather corresponds to the gain of the output
	// analog circuitry of the hardware units. However, together with setOutputGain() it offers to the user a capability
	// to control the gain of reverb and non-reverb output channels independently.
	//
	// Note: We're currently emulate CM-32L/CM-64 reverb quite accurately and the reverb output level closely
	// corresponds to the level of digital capture. Although, according to the CM-64 PCB schematic,
	// there is a difference in the reverb analogue circuit, and the resulting output gain is 0.68
	// of that for LA32 analogue output. This factor is applied to the reverb output gain.
	// Ignored in DACInputMode_PURE
	void setReverbOutputGain(float gain);
	// Returns current output gain factor for reverb wet output channels.
	float getReverbOutputGain() const;

	// Swaps left and right output channels.
	void setReversedStereoEnabled(bool enabled);
	// Returns whether left and right output channels are swapped.
	bool isReversedStereoEnabled() const;

	// Returns actual sample rate used in emulation of stereo analog circuitry of hardware units.
	// See comment for render() below.
	unsigned int getStereoOutputSampleRate() const;

	// Renders samples to the specified output stream as if they were sampled at the analog stereo output.
	// When AnalogOutputMode is set to ACCURATE (OVERSAMPLED), the output signal is upsampled to 48 (96) kHz in order
	// to retain emulation accuracy in whole audible frequency spectra. Otherwise, native digital signal sample rate is retained.
	// getStereoOutputSampleRate() can be used to query actual sample rate of the output signal.
	// The length is in frames, not bytes (in 16-bit stereo, one frame is 4 bytes). Uses NATIVE byte ordering.
	void render(Sample *stream, Bit32u len);

	// Renders samples to the specified output streams as if they appeared at the DAC entrance.
	// No further processing performed in analog circuitry emulation is applied to the signal.
	// NULL may be specified in place of any or all of the stream buffers to skip it.
	// The length is in samples, not bytes. Uses NATIVE byte ordering.
	void renderStreams(Sample *nonReverbLeft, Sample *nonReverbRight, Sample *reverbDryLeft, Sample *reverbDryRight, Sample *reverbWetLeft, Sample *reverbWetRight, Bit32u len);

	// Returns true when there is at least one active partial, otherwise false.
	bool hasActivePartials() const;

	// Returns true if hasActivePartials() returns true, or reverb is (somewhat unreliably) detected as being active.
	bool isActive() const;

	// Returns the maximum number of partials playing simultaneously.
	unsigned int getPartialCount() const;

	// Fills in current states of all the parts into the array provided. The array must have at least 9 entries to fit values for all the parts.
	// If the value returned for a part is true, there is at least one active non-releasing partial playing on this part.
	// This info is useful in emulating behaviour of LCD display of the hardware units.
	void getPartStates(bool *partStates) const;

	// Fills in current states of all the partials into the array provided. The array must be large enough to accommodate states of all the partials.
	void getPartialStates(PartialState *partialStates) const;

	// Fills in information about currently playing notes on the specified part into the arrays provided. The arrays must be large enough
	// to accommodate data for all the playing notes. The maximum number of simultaneously playing notes cannot exceed the number of partials.
	// Argument partNumber should be 0..7 for Part 1..8, or 8 for Rhythm.
	// Returns the number of currently playing notes on the specified part.
	unsigned int getPlayingNotes(unsigned int partNumber, Bit8u *keys, Bit8u *velocities) const;

	// Returns name of the patch set on the specified part.
	// Argument partNumber should be 0..7 for Part 1..8, or 8 for Rhythm.
	const char *getPatchName(unsigned int partNumber) const;

	// Stores internal state of emulated synth into an array provided (as it would be acquired from hardware).
	void readMemory(Bit32u addr, Bit32u len, Bit8u *data);
}; // class Synth

} // namespace MT32Emu

#endif // #ifndef MT32EMU_SYNTH_H
