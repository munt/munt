/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DemoPlayer.h"

#include <cstring>

#include "MasterClock.h"

using namespace MT32Emu;

static const uint SYSEX_ADDRESS_SIZE = 3;
static const uint SONG_TITLE_SIZE = 14;
static const uint REVERB_SETTINGS_SIZE = 3;
static const uint PARTIAL_RESERVE_SIZE = 9;
static const uint RHYTHM_SETUP_SIZE = 256;
static const uint PATCH_COUNT = 64;
static const uint PATCH_SIZE = 8;
static const uint PATCH_MEMORY_SIZE = PATCH_SIZE * PATCH_COUNT;
static const uint TIMBRE_COMMON_PARAMETERS_SIZE = 14;
static const uint TIMBRE_PARTIAL_MUTE_OFFSET = 12;
static const uint TIMBRE_PARTIAL_PARAMETERS_SIZE = 58;
static const uint MAX_SYSEX_SIZE = PATCH_MEMORY_SIZE + 10;

static const Bit32u RHYTHM_SETUP_SYSEX_ADDRESS = 0x030110;
static const Bit32u PATCH_MEMORY_SYSEX_ADDRESS = 0x050000;
static const Bit32u TIMBRE_MEMORY_SYSEX_ADDRESS = 0x080000;
static const Bit32u SYSTEM_REVERB_SYSEX_ADDRESS = 0x100001;
static const Bit32u DISPLAY_SYSEX_ADDRESS = 0x200000;
static const Bit32u SYSTEM_RESET_SYSEX_ADDRESS = 0x7F0000;

static const uint EXPECTED_ROM_SIZE = 131072;
static const uint DEMO_SONG_COUNT = 5;
static const uint DEMO_SONG_REFS = 0x86E0;
static const uint DEMO_TIMBRE_COUNT = 22;
static const uint DEMO_TIMBRE_ADDRESSES = 0x8700;
static const uint DEMO_PATCH_TABLE = 0x8800;

static const uint TITLE_OFFSET = 0x0000;
static const uint MIDI_TICK_OFFSET = 0x0010;
static const uint REVERB_SETTINGS_OFFSET = 0x0012;
static const uint RHYTHM_SETUP_OFFSET = 0x0020;
static const uint MIDI_STREAM_OFFSET = 0x0120;

static const Bit32u TIMER_FREQUENCY_HZ = 500000;

static const Bit8u SYSEX_HEADER[] = {0xF0, SYSEX_MANUFACTURER_ROLAND, 0x10, SYSEX_MDL_MT32, SYSEX_CMD_DT1};
static const Bit8u SYSEX_TERMINATOR = 0xF7;

static const uint DELAY_BEFORE_NEXT_SONG = 240;

static MasterClockNanos computeMidiTick(const Bit8u *songData) {
	return MasterClock::NANOS_PER_SECOND * qFromLittleEndian<quint16>(&songData[MIDI_TICK_OFFSET]) / TIMER_FREQUENCY_HZ;
}

static QString getSongName(const Bit8u *songData) {
	const char *songTitle = reinterpret_cast<const char *>(&songData[TITLE_OFFSET]);
	return QString::fromLatin1(songTitle, SONG_TITLE_SIZE).trimmed();
}

static void writeSysexAddr(Bit8u *&sysexPtr, Bit32u sysexAddress) {
	*(sysexPtr++) = (sysexAddress >> 16) & 0xFF;
	*(sysexPtr++) = (sysexAddress >> 8) & 0xFF;
	*(sysexPtr++) = sysexAddress & 0xFF;
}

static uint finishSysex(Bit8u *sysexAddr, uint length) {
	sysexAddr[length] = Synth::calcSysexChecksum(sysexAddr, length);
	sysexAddr[length + 1] = SYSEX_TERMINATOR;
	return sizeof SYSEX_HEADER + length + 2;
}

static uint checkROM(const MT32Emu::ROMImage *controlROMImage) {
	if (controlROMImage == NULL) return 0;
	const ROMInfo *romInfo = controlROMImage->getROMInfo();
	return ROMInfo::Control == romInfo->type && EXPECTED_ROM_SIZE == romInfo->fileSize ? DEMO_SONG_COUNT : 0;
}

const ROMImage *DemoPlayer::findSuitableROM(Master *master) {
	const QStringList synthProfiles = master->enumSynthProfiles();
	foreach (QString synthProfileName, synthProfiles) {
		SynthProfile synthProfile;
		master->loadSynthProfile(synthProfile, synthProfileName);
		if (!synthProfile.controlROMFileName2.isEmpty()) continue;
		FileStream *file = new FileStream;
		if (file->open(Master::getROMPathNameLocal(synthProfile.romDir, synthProfile.controlROMFileName))) {
			if (EXPECTED_ROM_SIZE == file->getSize()) {
				const ROMImage *romImage = ROMImage::makeROMImage(file, ROMInfo::getFullROMInfos());
				const ROMInfo *romInfo = romImage->getROMInfo();
				if (romInfo != NULL && ROMInfo::Control == romInfo->type) return romImage;
				ROMImage::freeROMImage(romImage);
			}
		}
		delete file;
	}
	return NULL;
}

DemoPlayer::DemoPlayer(Master *master, const MT32Emu::ROMImage *controlROMImage) :
	controlROMImage(controlROMImage), demoSongCount(checkROM(controlROMImage)), smfDriver(master)
{}

DemoPlayer::~DemoPlayer() {
	stop();
	if (controlROMImage != NULL) {
		if (controlROMImage->isFileUserProvided()) delete controlROMImage->getFile();
		ROMImage::freeROMImage(controlROMImage);
	}
}

const QString DemoPlayer::getStreamName() const {
	return midiStreamName;
}

const QMidiEventList &DemoPlayer::getMIDIEvents() const {
	return midiEventList;
}

MasterClockNanos DemoPlayer::getMidiTick(uint) const {
	return midiTick;
}

const QStringList DemoPlayer::getDemoSongs() const {
	QStringList demoSongs;
	for (uint songIx = 0; songIx < demoSongCount; songIx++) {
		demoSongs += getSongName(getSongData(songIx));
	}
	return demoSongs;
}

void DemoPlayer::playSong(uint songNumber) {
	stop();
	playbackDelayed = false;
	play(songNumber);
}

void DemoPlayer::chainPlay() {
	stop();
	playbackDelayed = false;
	connect(&smfDriver, SIGNAL(playbackFinished(bool)), SLOT(startNextSong(bool)));
	play(0);
}

void DemoPlayer::randomPlay() {
	stop();
	playbackDelayed = false;
	currentSongNumber = demoSongCount;
	connect(&smfDriver, SIGNAL(playbackFinished(bool)), SLOT(startRandomSong(bool)));
	startRandomSong(true);
}

void DemoPlayer::play(uint songNumber) {
	midiEventList.clear();
	if (demoSongCount <= songNumber) return;
	currentSongNumber = songNumber;
	songData = getSongData(songNumber);
	midiStreamName = "Demo " + QString().setNum(songNumber + 1) + ": " + getSongName(songData);
	midiTick = computeMidiTick(songData);
	configureSynth();
	parseMIDIEvents(&songData[MIDI_STREAM_OFFSET]);
	smfDriver.start(this);
}

const Bit8u *DemoPlayer::getSongData(uint songNumber) const {
	const Bit8u *romData = controlROMImage->getFile()->getData();
	quint32 songAbsoluteAddress = 2 * qFromLittleEndian<quint16>(&romData[DEMO_SONG_REFS + 2 * songNumber]) + 0x8000;
	return &romData[songAbsoluteAddress];
}

void DemoPlayer::configureSynth() {
	Bit8u sysex[MAX_SYSEX_SIZE];
	memcpy(sysex, SYSEX_HEADER, sizeof SYSEX_HEADER);
	Bit8u *sysexAddress = &sysex[sizeof SYSEX_HEADER];

	uint length = finishSysex(sysexAddress, makeSongTitleSysex(sysexAddress));
	addSysex(sysex, length);
	if (playbackDelayed) midiEventList.newMidiEvent().assignSyncMessage(DELAY_BEFORE_NEXT_SONG);
	length = finishSysex(sysexAddress, makeSystemResetSysex(sysexAddress));
	addSysex(sysex, length);
	length = finishSysex(sysexAddress, makeSystemSettingsSysex(sysexAddress));
	addSysex(sysex, length);
	length = finishSysex(sysexAddress, makeRhythmSetupSysex(sysexAddress));
	addSysex(sysex, length);
	for (uint timbreNumber = 0; timbreNumber < DEMO_TIMBRE_COUNT; timbreNumber++) {
		length = finishSysex(sysexAddress, makeTimbreSysex(sysexAddress, timbreNumber));
		addSysex(sysex, length);
	}
	length = finishSysex(sysexAddress, makePatchMemorySysex(sysexAddress));
	addSysex(sysex, length);
}

Bit32u DemoPlayer::makeSongTitleSysex(Bit8u *sysexPtr) {
	writeSysexAddr(sysexPtr, DISPLAY_SYSEX_ADDRESS);
	*(sysexPtr++) = '1' + currentSongNumber;
	*(sysexPtr++) = ':';
	memcpy(sysexPtr, songData + TITLE_OFFSET, SONG_TITLE_SIZE);
	sysexPtr += SONG_TITLE_SIZE;
	*(sysexPtr++) = '|';
	*(sysexPtr++) = 0;
	return SYSEX_ADDRESS_SIZE + SONG_TITLE_SIZE + 4;
}

Bit32u DemoPlayer::makeSystemResetSysex(Bit8u *sysexPtr) {
	writeSysexAddr(sysexPtr, SYSTEM_RESET_SYSEX_ADDRESS);
	return SYSEX_ADDRESS_SIZE;
}

Bit32u DemoPlayer::makeSystemSettingsSysex(Bit8u *sysexPtr) {
	writeSysexAddr(sysexPtr, SYSTEM_REVERB_SYSEX_ADDRESS);
	memcpy(sysexPtr, songData + REVERB_SETTINGS_OFFSET, REVERB_SETTINGS_SIZE + PARTIAL_RESERVE_SIZE);
	return SYSEX_ADDRESS_SIZE + REVERB_SETTINGS_SIZE + PARTIAL_RESERVE_SIZE;
}

Bit32u DemoPlayer::makeTimbreSysex(Bit8u *sysexPtr, uint timbreNumber) {
	writeSysexAddr(sysexPtr, 0x200 * timbreNumber + TIMBRE_MEMORY_SYSEX_ADDRESS);
	const Bit8u *romData = controlROMImage->getFile()->getData();
	quint32 timbreAbsoluteAddress = qFromLittleEndian<quint16>(&romData[2 * timbreNumber + DEMO_TIMBRE_ADDRESSES]);
	const Bit8u *romPtr = &romData[timbreAbsoluteAddress];
	Bit8u partialMute = romPtr[TIMBRE_PARTIAL_MUTE_OFFSET];
	memcpy(sysexPtr, romPtr, TIMBRE_COMMON_PARAMETERS_SIZE);
	sysexPtr += TIMBRE_COMMON_PARAMETERS_SIZE;
	romPtr += TIMBRE_COMMON_PARAMETERS_SIZE;
	for (uint partialIx = 0; partialIx < 4; partialIx++) {
		memcpy(sysexPtr, romPtr, TIMBRE_PARTIAL_PARAMETERS_SIZE);
		sysexPtr += TIMBRE_PARTIAL_PARAMETERS_SIZE;
		if (partialIx == 0 || (partialMute & (1 << partialIx)) != 0) romPtr += TIMBRE_PARTIAL_PARAMETERS_SIZE;
	}
	return SYSEX_ADDRESS_SIZE + TIMBRE_COMMON_PARAMETERS_SIZE + (4 * TIMBRE_PARTIAL_PARAMETERS_SIZE);
}

Bit32u DemoPlayer::makePatchMemorySysex(Bit8u *sysexPtr) {
	writeSysexAddr(sysexPtr, PATCH_MEMORY_SYSEX_ADDRESS);
	const Bit8u *romData = controlROMImage->getFile()->getData();
	memcpy(sysexPtr, romData + DEMO_PATCH_TABLE, PATCH_MEMORY_SIZE);
	// Some of the demo patches refer to timbre group 5 which we here map to memory timbres.
	Bit8u *sysexEndPtr = sysexPtr + PATCH_MEMORY_SIZE;
	while (sysexPtr < sysexEndPtr) {
		if (*sysexPtr == 5) *sysexPtr = 2;
		sysexPtr += PATCH_SIZE;
	}
	return SYSEX_ADDRESS_SIZE + PATCH_MEMORY_SIZE;
}

Bit32u DemoPlayer::makeRhythmSetupSysex(Bit8u *sysexPtr) {
	writeSysexAddr(sysexPtr, RHYTHM_SETUP_SYSEX_ADDRESS);
	memcpy(sysexPtr, songData + RHYTHM_SETUP_OFFSET, RHYTHM_SETUP_SIZE);
	return SYSEX_ADDRESS_SIZE + RHYTHM_SETUP_SIZE;
}

void DemoPlayer::parseMIDIEvents(const Bit8u *songDataPtr) {
	Bit8u runningStatus = 0;
	forever {
		Bit8u delay = *(songDataPtr++);
		if (delay == 0xFF) break;
		if (delay == 0xF8) {
			midiEventList.newMidiEvent().assignSyncMessage(delay);
			continue;
		}
		Bit8u byte = *(songDataPtr++);
		if (byte == 0xFC) break;
		if ((byte & 0xF0) == 0xF0) {
			midiEventList.newMidiEvent().assignSyncMessage(delay);
			continue;
		}
		Bit32u message;
		if ((byte & 0x80) != 0) {
			message = byte;
			runningStatus = byte;
			message |= (*(songDataPtr++) << 8);
		} else if (runningStatus != 0) {
			message = runningStatus | (byte << 8);
		} else continue;
		if ((message & 0xE0) != 0xC0) {
			message |= (*(songDataPtr++) << 16);
		}
		midiEventList.newMidiEvent().assignShortMessage(delay, message);
	}
}

void DemoPlayer::addSysex(Bit8u *sysex, uint length) {
	midiEventList.newMidiEvent().assignSysex(0, sysex, length);
}

void DemoPlayer::stop() {
	smfDriver.disconnect(this);
	smfDriver.stop();
}

void DemoPlayer::startNextSong(bool enabled) {
	if (!enabled) {
		smfDriver.disconnect(this);
		return;
	}
	playbackDelayed = true;
	play(++currentSongNumber % demoSongCount);
}

void DemoPlayer::startRandomSong(bool enabled) {
	if (!enabled) {
		smfDriver.disconnect(this);
		return;
	}
	uint nextSongNumber;
	do {
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
		nextSongNumber = uint(qrand()) % demoSongCount;
#else
		nextSongNumber = QRandomGenerator::global()->bounded(demoSongCount);
#endif
	} while(nextSongNumber == currentSongNumber);
	play(nextSongNumber);
	playbackDelayed = true;
}
