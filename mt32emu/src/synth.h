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

#ifndef MT32EMU_SYNTH_H
#define MT32EMU_SYNTH_H

#include <stdarg.h>

class revmodel;

namespace MT32Emu {

class File;
class TableInitialiser;
class Partial;
class PartialManager;
class Part;

enum ReportType {
	// Errors
	ReportType_errorControlROM = 1,
	ReportType_errorPCMROM,
	ReportType_errorSampleRate,

	// Progress
	ReportType_progressInit,

	// HW spec
	ReportType_availableSSE,
	ReportType_available3DNow,
	ReportType_usingSSE,
	ReportType_using3DNow,

	// General info
	ReportType_lcdMessage,
	ReportType_devReset,
	ReportType_devReconfig,
	ReportType_newReverbMode,
	ReportType_newReverbTime,
	ReportType_newReverbLevel
};

enum LoadResult {
	LoadResult_OK,
	LoadResult_NotFound,
	LoadResult_Unreadable,
	LoadResult_Invalid
};

struct SynthProperties {
	// Sample rate to use in mixing
	unsigned int sampleRate;

	// Deprecated - ignored. Use Synth::setReverbEnabled() instead.
	bool useReverb;
	// Deprecated - ignored. Use Synth::setReverbOverridden() instead.
	bool useDefaultReverb;
	// Deprecated - ignored. Use Synth::setReverbParameters() instead.
	unsigned char reverbType;
	// Deprecated - ignored. Use Synth::setReverbParameters() instead.
	unsigned char reverbTime;
	// Deprecated - ignored. Use Synth::setReverbParameters() instead.
	unsigned char reverbLevel;
	// The name of the directory in which the ROM and data files are stored (with trailing slash/backslash)
	// Not used if "openFile" is set. May be NULL in any case.
	char *baseDir;
	// This is used as the first argument to all callbacks
	void *userData;
	// Callback for reporting various errors and information. May be NULL
	int (*report)(void *userData, ReportType type, const void *reportData);
	// Callback for debug messages, in vprintf() format
	void (*printDebug)(void *userData, const char *fmt, va_list list);
	// Callback for providing an implementation of File, opened and ready for use
	// May be NULL, in which case a default implementation will be used.
	File *(*openFile)(void *userData, const char *filename, File::OpenMode mode);
	// Callback for closing a File. May be NULL, in which case the File will automatically be close()d/deleted.
	void (*closeFile)(void *userData, File *file);
};

// This is the specification of the Callback routine used when calling the RecalcWaveforms
// function
typedef void (*recalcStatusCallback)(int percDone);

// This external function recreates the base waveform file (waveforms.raw) using a specifed
// sampling rate.  The callback routine provides interactivity to let the user know what
// percentage is complete in regenerating the waveforms.  When a NULL pointer is used as the
// callback routine, no status is reported.
bool RecalcWaveforms(char * baseDir, int sampRate, recalcStatusCallback callBack);

typedef float (*iir_filter_type)(float input,float *hist1_ptr, const float *coef_ptr);

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

const int MAX_SYSEX_SIZE = 512;

const unsigned int CONTROL_ROM_SIZE = 64 * 1024;

struct ControlROMPCMStruct
{
	Bit8u pos;
	Bit8u len;
	Bit8u pitchLSB;
	Bit8u pitchMSB;
};

struct ControlROMMap {
	Bit16u idPos;
	Bit16u idLen;
	const char *idBytes;
	Bit16u pcmTable; // 4 * pcmCount bytes
	Bit16u pcmCount;
	Bit16u timbreAMap; // 128 bytes
	Bit16u timbreAOffset;
	bool timbreACompressed;
	Bit16u timbreBMap; // 128 bytes
	Bit16u timbreBOffset;
	bool timbreBCompressed;
	Bit16u timbreRMap; // 2 * timbreRCount bytes
	Bit16u timbreRCount;
	Bit16u rhythmSettings; // 4 * rhythmSettingsCount bytes
	Bit16u rhythmSettingsCount;
	Bit16u reserveSettings; // 9 bytes
	Bit16u panSettings; // 8 bytes
	Bit16u programSettings; // 8 bytes
	Bit16u rhythmMaxTable; // 4 bytes
	Bit16u patchMaxTable; // 16 bytes
	Bit16u systemMaxTable; // 23 bytes
	Bit16u timbreMaxTable; // 72 bytes
};

enum MemoryRegionType {
	MR_PatchTemp, MR_RhythmTemp, MR_TimbreTemp, MR_Patches, MR_Timbres, MR_System, MR_Display, MR_Reset
};

class MemoryRegion {
private:
	Synth *synth;
	Bit8u *realMemory;
	Bit8u *maxTable;
public:
	MemoryRegionType type;
	Bit32u startAddr, entrySize, entries;

	MemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable, MemoryRegionType type, Bit32u startAddr, Bit32u entrySize, Bit32u entries) {
		this->synth = synth;
		this->realMemory = realMemory;
		this->maxTable = maxTable;
		this->type = type;
		this->startAddr = startAddr;
		this->entrySize = entrySize;
		this->entries = entries;
	}
	int lastTouched(Bit32u addr, Bit32u len) const {
		return (offset(addr) + len - 1) / entrySize;
	}
	int firstTouchedOffset(Bit32u addr) const {
		return offset(addr) % entrySize;
	}
	int firstTouched(Bit32u addr) const {
		return offset(addr) / entrySize;
	}
	Bit32u regionEnd() const {
		return startAddr + entrySize * entries;
	}
	bool contains(Bit32u addr) const {
		return addr >= startAddr && addr < regionEnd();
	}
	int offset(Bit32u addr) const {
		return addr - startAddr;
	}
	Bit32u getClampedLen(Bit32u addr, Bit32u len) const {
		if (addr + len > regionEnd())
			return regionEnd() - addr;
		return len;
	}
	Bit32u next(Bit32u addr, Bit32u len) const {
		if (addr + len > regionEnd()) {
			return regionEnd() - addr;
		}
		return 0;
	}
	Bit8u getMaxValue(int off) const {
		if (maxTable == NULL)
			return 0xFF;
		return maxTable[off % entrySize];
	}
	Bit8u *getRealMemory() const {
		return realMemory;
	}
	bool isReadable() const {
		return getRealMemory() != NULL;
	}
	void read(unsigned int entry, unsigned int off, Bit8u *dst, unsigned int len) const;
	void write(unsigned int entry, unsigned int off, const Bit8u *src, unsigned int len, bool init = false) const;
};

class PatchTempMemoryRegion : public MemoryRegion {
public:
	PatchTempMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_PatchTemp, MT32EMU_MEMADDR(0x030000), sizeof(MemParams::PatchTemp), 9) {}
};
class RhythmTempMemoryRegion : public MemoryRegion {
public:
	RhythmTempMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_RhythmTemp, MT32EMU_MEMADDR(0x030110), sizeof(MemParams::RhythmTemp), 85) {}
};
class TimbreTempMemoryRegion : public MemoryRegion {
public:
	TimbreTempMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_TimbreTemp, MT32EMU_MEMADDR(0x040000), sizeof(TimbreParam), 8) {}
};
class PatchesMemoryRegion : public MemoryRegion {
public:
	PatchesMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_Patches, MT32EMU_MEMADDR(0x050000), sizeof(PatchParam), 128) {}
};
class TimbresMemoryRegion : public MemoryRegion {
public:
	TimbresMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_Timbres, MT32EMU_MEMADDR(0x080000), sizeof(MemParams::PaddedTimbre), 64 + 64 + 64 + 64) {}
};
class SystemMemoryRegion : public MemoryRegion {
public:
	SystemMemoryRegion(Synth *synth, Bit8u *realMemory, Bit8u *maxTable) : MemoryRegion(synth, realMemory, maxTable, MR_System, MT32EMU_MEMADDR(0x100000), sizeof(MemParams::System), 1) {}
};
class DisplayMemoryRegion : public MemoryRegion {
public:
	DisplayMemoryRegion(Synth *synth) : MemoryRegion(synth, NULL, NULL, MR_Display, MT32EMU_MEMADDR(0x200000), MAX_SYSEX_SIZE - 1, 1) {}
};
class ResetMemoryRegion : public MemoryRegion {
public:
	ResetMemoryRegion(Synth *synth) : MemoryRegion(synth, NULL, NULL, MR_Reset, MT32EMU_MEMADDR(0x7F0000), 0x3FFF, 1) {}
};

class ReverbModel {
public:
	virtual ~ReverbModel() {};
	virtual void setSampleRate(unsigned int sampleRate) = 0;
	virtual void setParameters(Bit8u mode, Bit8u time, Bit8u level) = 0;
	virtual void process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, long numSamples) = 0;
	virtual void reset() = 0;
};

class FreeverbModel : public ReverbModel {
	revmodel *freeverb;
public:
	FreeverbModel();
	~FreeverbModel();
	void setSampleRate(unsigned int sampleRate);
	void setParameters(Bit8u mode, Bit8u time, Bit8u level);
	void process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, long numSamples);
	void reset();
};

class Synth {
friend class Part;
friend class RhythmPart;
friend class Partial;
friend class Tables;
friend class MemoryRegion;
friend class TVA;
friend class TVP;
friend class TVF;
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

	iir_filter_type iirFilter;

	PCMWaveEntry *pcmWaves; // Array

	const ControlROMMap *controlROMMap;
	Bit8u controlROMData[CONTROL_ROM_SIZE];
	Bit16s *pcmROMData;
	int pcmROMSize; // This is in 16-bit samples, therefore half the number of bytes in the ROM

	Bit8s chantable[32];

	#if MT32EMU_MONITOR_PARTIALS == 1
	static Bit32s samplepos = 0;
	#endif

	Tables tables;

	MemParams mt32ram, mt32default;

	ReverbModel *reverbModel;
	ReverbModel *delayReverbModel;
	bool reverbEnabled;
	bool reverbOverridden;

	float masterTune;

	bool isOpen;

	PartialManager *partialManager;
	Part *parts[9];

	Bit16s tmpBuffer[MAX_SAMPLE_OUTPUT * 2];
	float sndbufl[MAX_SAMPLE_OUTPUT];
	float sndbufr[MAX_SAMPLE_OUTPUT];
	float outbufl[MAX_SAMPLE_OUTPUT];
	float outbufr[MAX_SAMPLE_OUTPUT];

	SynthProperties myProp;

	bool loadPreset(File *file);
	void doRender(Bit16s * stream, Bit32u len);

	void playAddressedSysex(unsigned char channel, const Bit8u *sysex, Bit32u len);
	void readSysex(unsigned char channel, const Bit8u *sysex, Bit32u len);
	void initMemoryRegions();
	void deleteMemoryRegions();
	MemoryRegion *findMemoryRegion(Bit32u addr);
	void writeMemoryRegion(const MemoryRegion *region, Bit32u addr, Bit32u len, const Bit8u *data);
	void readMemoryRegion(const MemoryRegion *region, Bit32u addr, Bit32u len, Bit8u *data);

	LoadResult loadControlROM(const char *filename);
	LoadResult loadPCMROM(const char *filename);
	bool dumpTimbre(File *file, const TimbreParam *timbre, Bit32u addr);
	int dumpTimbres(const char *filename, int start, int len);

	bool initPCMList(Bit16u mapAddress, Bit16u count);
	bool initTimbres(Bit16u mapAddress, Bit16u offset, int timbreCount, int startTimbre, bool compressed);
	bool initCompressedTimbre(int drumNum, const Bit8u *mem, unsigned int memLen);
	bool refreshSystem();
	void reset();

	unsigned int getSampleRate() const;
protected:
	int report(ReportType type, const void *reportData);
	File *openFile(const char *filename, File::OpenMode mode);
	void closeFile(File *file);
	void printDebug(const char *fmt, ...);

public:
	static Bit8u calcSysexChecksum(const Bit8u *data, Bit32u len, Bit8u checksum);

	Synth();
	~Synth();

	// Used to initialise the MT-32. Must be called before any other function.
	// Returns true if initialization was sucessful, otherwise returns false.
	bool open(SynthProperties &useProp);

	// Closes the MT-32 and deallocates any memory used by the synthesizer
	void close(void);

	// Sends a 4-byte MIDI message to the MT-32 for immediate playback
	void playMsg(Bit32u msg);
	void playMsgOnPart(unsigned char part, unsigned char code, unsigned char note, unsigned char velocity);

	// Sends a string of Sysex commands to the MT-32 for immediate interpretation
	// The length is in bytes
	void playSysex(const Bit8u *sysex, Bit32u len);
	void playSysexWithoutFraming(const Bit8u *sysex, Bit32u len);
	void playSysexWithoutHeader(unsigned char device, unsigned char command, const Bit8u *sysex, Bit32u len);
	void writeSysex(unsigned char channel, const Bit8u *sysex, Bit32u len);

	void setReverbModel(ReverbModel *reverbModel);
	void setDelayReverbModel(ReverbModel *reverbModel);
	void setReverbEnabled(bool reverbEnabled);
	bool isReverbEnabled() const;
	void setReverbOverridden(bool reverbOverridden);
	bool isReverbOverridden() const;
	void setReverbParameters(Bit8u mode, Bit8u time, Bit8u level);

	// Renders samples to the specified output stream.
	// The length is in frames, not bytes (in 16-bit stereo,
	// one frame is 4 bytes).
	void render(Bit16s *stream, Bit32u len);

	// Returns true when there is at least one active partial, otherwise false.
	bool isActive() const;

	const Partial *getPartial(unsigned int partialNum) const;

	void readMemory(Bit32u addr, Bit32u len, Bit8u *data);

	// partNum should be 0..7 for Part 1..8, or 8 for Rhythm
	const Part *getPart(unsigned int partNum) const;
};

}

#endif
