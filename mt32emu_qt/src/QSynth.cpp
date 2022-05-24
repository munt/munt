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

#include <cstring>
#include <QtGlobal>
#include <QMessageBox>

#include "QSynth.h"
#include "AudioFileWriter.h"
#include "Master.h"
#include "MasterClock.h"
#include "RealtimeLocker.h"

using namespace MT32Emu;

const int INITIAL_MASTER_VOLUME = 100;
const int LCD_MESSAGE_LENGTH = 21; // 0-terminated
const int MIN_PARTIAL_COUNT = 8;
const int MAX_PARTIAL_COUNT = 256;
const int VOICE_PART_COUNT = 8;
const int PART_COUNT = VOICE_PART_COUNT + 1;
const int SOUND_GROUP_NAME_LENGTH = 9; // 0-terminated
const int TIMBRE_NAME_LENGTH = 11; // 0-terminated
const int NO_UPDATE_VALUE = -1;

static const ROMImage *makeROMImage(const QDir &romDir, QString romFileName, QString romFileName2) {
	if (romFileName2.isEmpty()) {
		FileStream *file = new FileStream;
		if (file->open(Master::getROMPathNameLocal(romDir, romFileName))) {
			const ROMImage *romImage = ROMImage::makeROMImage(file, ROMInfo::getFullROMInfos());
			if (romImage->getROMInfo() != NULL) return romImage;
			ROMImage::freeROMImage(romImage);
		}
		delete file;
		return NULL;
	}
	FileStream file1;
	FileStream file2;
	if (!file1.open(Master::getROMPathNameLocal(romDir, romFileName))) return NULL;
	if (!file2.open(Master::getROMPathNameLocal(romDir, romFileName2))) return NULL;
	return ROMImage::makeROMImage(&file1, &file2);
}

static void writeMasterVolumeSysex(Synth *synth, int masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, (Bit8u)masterVolume};
	synth->writeSysex(16, sysex, 4);
}

static void overrideReverbSettings(Synth *synth, int reverbMode, int reverbTime, int reverbLevel) {
	Bit8u sysex[] = {0x10, 0x00, 0x01, (Bit8u)reverbMode, (Bit8u)reverbTime, (Bit8u)reverbLevel};
	synth->setReverbOverridden(false);
	synth->writeSysex(16, sysex, 6);
	synth->setReverbOverridden(true);
}

static void writeMIDIChannelsAssignmentResetSysex(Synth *synth, bool engageChannel1) {
	static const Bit8u sysexStandardChannelAssignment[] = {0x10, 0x00, 0x0d, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
	static const Bit8u sysexChannel1EngagedAssignment[] = {0x10, 0x00, 0x0d, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};

	synth->writeSysex(16, engageChannel1 ? sysexChannel1EngagedAssignment : sysexStandardChannelAssignment, sizeof(sysexStandardChannelAssignment));
}

static void writeSystemResetSysex(Synth *synth) {
	static const Bit8u sysex[] = {0x7f, 0, 0};
	synth->writeSysex(16, sysex, 3);
}

static int readMasterVolume(Synth *synth) {
	Bit8u masterVolume = 0;
	synth->readMemory(0x40016, 1, &masterVolume);
	return masterVolume;
}

static void writeTimbreSelectionOnPartSysex(Synth *synth, quint8 partNumber, quint8 timbreGroup, quint8 timbreNumber) {
	Bit8u sysex[] = {0x03, 0x00, quint8(partNumber << 4), timbreGroup, timbreNumber};
	synth->writeSysex(16, sysex, 5);
}

static void makeSoundGroups(Synth &synth, QVector<SoundGroup> &groups) {
	SoundGroup *group = NULL;
	for (uint timbreGroup = 0; timbreGroup < 4; timbreGroup++) {
		for (uint timbreNumber = 0; timbreNumber < 64; timbreNumber++) {
			char soundName[TIMBRE_NAME_LENGTH];
			if (!synth.getSoundName(soundName, timbreGroup, timbreNumber)) continue;
			char groupName[SOUND_GROUP_NAME_LENGTH];
			if (!synth.getSoundGroupName(groupName, timbreGroup, timbreNumber)) continue;
			if (group == NULL || group->name != groupName) {
				groups.resize(groups.size() + 1U);
				group = &groups.last();
				group->name = groupName;
			}
			SoundGroup::Item item = {timbreGroup, timbreNumber, soundName};
			group->constituents += item;
		}
	}
}

class RealtimeHelper : public QThread {
private:
	enum SynthControlEvent {
		SYNTH_RESET,
		MASTER_VOLUME_CHANGED,
		OUTPUT_GAIN_CHANGED,
		REVERB_OUTPUT_GAIN_CHANGED,
		REVERB_ENABLED_CHANGED,
		REVERB_OVERRIDDEN_CHANGED,
		REVERB_SETTINGS_CHANGED,
		PART_VOLUME_OVERRIDE_CHANGED,
		PART_TIMBRE_CHANGED,
		REVERSED_STEREO_ENABLED_CHANGED,
		NICE_AMP_RAMP_ENABLED_CHANGED,
		NICE_PANNING_ENABLED_CHANGED,
		NICE_PARTIAL_MIXING_ENABLED_CHANGED,
		EMU_DAC_INPUT_MODE_CHANGED,
		MIDI_DELAY_MODE_CHANGED,
		MIDI_CHANNELS_ASSIGNMENT_RESET,
		DISPLAY_RESET,
		DISPLAY_COMPATIBILITY_MODE_CHANGED
	};

	QSynth &qsynth;
	bool stopProcessing;

	QQueue<SynthControlEvent> synthControlEvents;

	// Synth settings, guarded by settingsMutex.
	int masterVolume;
	float outputGain;
	float reverbOutputGain;
	bool reverbEnabled;
	bool reverbOverridden;
	int partVolumeOverride[PART_COUNT];
	struct {
		quint8 changed : 1;
		quint8 group : 2;
		quint8 number : 6;
	} partTimbres[VOICE_PART_COUNT];
	bool reversedStereoEnabled;
	bool niceAmpRampEnabled;
	bool nicePanningEnabled;
	bool nicePartialMixingEnabled;
	DACInputMode emuDACInputMode;
	MIDIDelayMode midiDelayMode;
	bool midiChannelsAssignmentChannel1Engaged;
	DisplayCompatibilityMode displayCompatibilityMode;

	// Temp synth state collected while rendering, only accessed from the rendering thread.
	// On backpressure, the latest values are kept.
	struct {
		char lcdMessage[LCD_MESSAGE_LENGTH];
		bool lcdStateUpdated;
		bool midiMessageLEDState;
		bool midiMessageLEDStateUpdated;
		int masterVolumeUpdate;
		int reverbMode;
		int reverbTime;
		int reverbLevel;
		struct {
			bool polyStateChanged;
			bool programChanged;
			char soundGroupName[SOUND_GROUP_NAME_LENGTH];
			char timbreName[TIMBRE_NAME_LENGTH];
		} partStates[PART_COUNT];
	} tempState;

	// Synth state snapshot, guarded by stateSnapshotMutex.
	struct {
		char lcdMessage[LCD_MESSAGE_LENGTH];
		char lcdState[LCD_MESSAGE_LENGTH];
		bool lcdStateUpdated;
		bool midiMessageLEDState;
		bool midiMessageLEDStateUpdated;
		int masterVolumeUpdate;
		int reverbMode;
		int reverbTime;
		int reverbLevel;
		struct {
			bool polyStateChanged;
			bool programChanged;
			char soundGroupName[SOUND_GROUP_NAME_LENGTH];
			char timbreName[TIMBRE_NAME_LENGTH];
			Bit32u playingNotesCount;
			Bit8u keysOfPlayingNotes[MAX_PARTIAL_COUNT];
			Bit8u velocitiesOfPlayingNotes[MAX_PARTIAL_COUNT];
		} partStates[PART_COUNT];
		PartialState partialStates[MAX_PARTIAL_COUNT];
	} stateSnapshot;

	/** Ensures atomicity of collecting changes to be applied to the synth settings. */
	QMutex settingsMutex;
	/** Ensures atomicity of handling the output signals of the synth and capturing its internal state. */
	QMutex stateSnapshotMutex;
	/** Used to block this thread until each rendering pass completes. */
	QWaitCondition renderCompleteCondition;

	QVector<SoundGroup> soundGroupCache;

	void applyChangesRealtime() {
		RealtimeLocker settingsLocker(settingsMutex);
		if (!settingsLocker.isLocked()) return;
		Synth *synth = qsynth.synth;
		while (!synthControlEvents.isEmpty()) {
			switch (synthControlEvents.dequeue()) {
			case SYNTH_RESET:
				writeSystemResetSysex(synth);
				break;
			case MASTER_VOLUME_CHANGED:
				writeMasterVolumeSysex(synth, masterVolume);
				break;
			case OUTPUT_GAIN_CHANGED:
				synth->setOutputGain(outputGain);
				break;
			case REVERB_OUTPUT_GAIN_CHANGED:
				synth->setReverbOutputGain(reverbOutputGain);
				break;
			case REVERB_ENABLED_CHANGED:
				synth->setReverbEnabled(reverbEnabled);
				break;
			case REVERB_OVERRIDDEN_CHANGED:
				synth->setReverbOverridden(reverbOverridden);
				break;
			case REVERB_SETTINGS_CHANGED:
				overrideReverbSettings(synth, qsynth.reverbMode, qsynth.reverbTime, qsynth.reverbLevel);
				break;
			case PART_VOLUME_OVERRIDE_CHANGED:
				for (int i = 0; i < PART_COUNT; i++) {
					if (NO_UPDATE_VALUE != partVolumeOverride[i]) {
						synth->setPartVolumeOverride(i, partVolumeOverride[i]);
						partVolumeOverride[i] = NO_UPDATE_VALUE;
					}
				}
				break;
			case PART_TIMBRE_CHANGED:
				for (int i = 0; i < VOICE_PART_COUNT; i++) {
					if (partTimbres[i].changed) {
						partTimbres[i].changed = 0;
						writeTimbreSelectionOnPartSysex(synth, i, partTimbres[i].group, partTimbres[i].number);
					}
				}
				break;
			case REVERSED_STEREO_ENABLED_CHANGED:
				synth->setReversedStereoEnabled(reversedStereoEnabled);
				break;
			case NICE_AMP_RAMP_ENABLED_CHANGED:
				synth->setNiceAmpRampEnabled(niceAmpRampEnabled);
				break;
			case NICE_PANNING_ENABLED_CHANGED:
				synth->setNicePanningEnabled(nicePanningEnabled);
				break;
			case NICE_PARTIAL_MIXING_ENABLED_CHANGED:
				synth->setNicePartialMixingEnabled(nicePartialMixingEnabled);
				break;
			case EMU_DAC_INPUT_MODE_CHANGED:
				synth->setDACInputMode(emuDACInputMode);
				break;
			case MIDI_DELAY_MODE_CHANGED:
				synth->setMIDIDelayMode(midiDelayMode);
				break;
			case MIDI_CHANNELS_ASSIGNMENT_RESET:
				writeMIDIChannelsAssignmentResetSysex(synth, midiChannelsAssignmentChannel1Engaged);
				break;
			case DISPLAY_RESET:
				synth->setMainDisplayMode();
				break;
			case DISPLAY_COMPATIBILITY_MODE_CHANGED:
				if (DisplayCompatibilityMode_DEFAULT == displayCompatibilityMode) {
					synth->setDisplayCompatibility(synth->isDefaultDisplayOldMT32Compatible());
				} else {
					synth->setDisplayCompatibility(DisplayCompatibilityMode_OLD_MT32 == displayCompatibilityMode);
				}
				break;
			}
		}
	}

	void saveStateRealtime() {
		RealtimeLocker stateSnapshotLocker(stateSnapshotMutex);
		if (!stateSnapshotLocker.isLocked()) return;

		memcpy(stateSnapshot.lcdMessage, tempState.lcdMessage, LCD_MESSAGE_LENGTH - 1);
		tempState.lcdMessage[0] = 0;

		stateSnapshot.masterVolumeUpdate = tempState.masterVolumeUpdate;
		tempState.masterVolumeUpdate = NO_UPDATE_VALUE;

		stateSnapshot.reverbMode = tempState.reverbMode;
		tempState.reverbMode = NO_UPDATE_VALUE;

		stateSnapshot.reverbTime = tempState.reverbTime;
		tempState.reverbTime = NO_UPDATE_VALUE;

		stateSnapshot.reverbLevel = tempState.reverbLevel;
		tempState.reverbLevel = NO_UPDATE_VALUE;

		stateSnapshot.midiMessageLEDStateUpdated = tempState.midiMessageLEDStateUpdated;
		if (tempState.midiMessageLEDStateUpdated) {
			tempState.midiMessageLEDStateUpdated = false;
			stateSnapshot.midiMessageLEDState = tempState.midiMessageLEDState;
		}

		Synth *synth = qsynth.synth;

		stateSnapshot.lcdStateUpdated = tempState.lcdStateUpdated;
		if (tempState.lcdStateUpdated) {
			tempState.lcdStateUpdated = false;
			synth->getDisplayState(stateSnapshot.lcdState);
		}

		for (int partIx = 0; partIx < PART_COUNT; partIx++) {
			stateSnapshot.partStates[partIx].programChanged = tempState.partStates[partIx].programChanged;
			if (tempState.partStates[partIx].programChanged) {
				tempState.partStates[partIx].programChanged = false;
				memcpy(stateSnapshot.partStates[partIx].soundGroupName, tempState.partStates[partIx].soundGroupName, SOUND_GROUP_NAME_LENGTH - 1);
				memcpy(stateSnapshot.partStates[partIx].timbreName, tempState.partStates[partIx].timbreName, TIMBRE_NAME_LENGTH - 1);
			}

			stateSnapshot.partStates[partIx].polyStateChanged = tempState.partStates[partIx].polyStateChanged;
			if (tempState.partStates[partIx].polyStateChanged) {
				tempState.partStates[partIx].polyStateChanged = false;
				stateSnapshot.partStates[partIx].playingNotesCount = synth->getPlayingNotes(partIx, stateSnapshot.partStates[partIx].keysOfPlayingNotes, stateSnapshot.partStates[partIx].velocitiesOfPlayingNotes);
			}
		}

		synth->getPartialStates(stateSnapshot.partialStates);
	}

	void enqueueSynthControlEvent(SynthControlEvent value) {
		synthControlEvents.removeOne(value);
		synthControlEvents.enqueue(value);
	}

	void run() {
		QMutexLocker stateSnapshotLocker(&stateSnapshotMutex);
		QReportHandler &reportHandler = qsynth.reportHandler;
		while (renderCompleteCondition.wait(&stateSnapshotMutex) && !stopProcessing) {
			if (stateSnapshot.lcdMessage[0]) {
				reportHandler.doShowLCDMessage(stateSnapshot.lcdMessage);
				stateSnapshot.lcdMessage[0] = 0;
			}

			if (stateSnapshot.lcdStateUpdated) {
				emit reportHandler.lcdStateChanged();
				stateSnapshot.lcdStateUpdated = false;
			}

			if (stateSnapshot.midiMessageLEDStateUpdated) {
				emit reportHandler.midiMessageLEDStateChanged(stateSnapshot.midiMessageLEDState);
				stateSnapshot.midiMessageLEDStateUpdated = false;
			}

			if (stateSnapshot.masterVolumeUpdate > NO_UPDATE_VALUE) {
				emit reportHandler.masterVolumeChanged(stateSnapshot.masterVolumeUpdate);
				stateSnapshot.masterVolumeUpdate = NO_UPDATE_VALUE;
			}

			if (stateSnapshot.reverbMode > NO_UPDATE_VALUE) {
				emit reportHandler.reverbModeChanged(stateSnapshot.reverbMode);
				stateSnapshot.reverbMode = NO_UPDATE_VALUE;
			}

			if (stateSnapshot.reverbTime > NO_UPDATE_VALUE) {
				emit reportHandler.reverbTimeChanged(stateSnapshot.reverbTime);
				stateSnapshot.reverbTime = NO_UPDATE_VALUE;
			}

			if (stateSnapshot.reverbLevel > NO_UPDATE_VALUE) {
				emit reportHandler.reverbLevelChanged(stateSnapshot.reverbLevel);
				stateSnapshot.reverbLevel = NO_UPDATE_VALUE;
			}

			for (int partIx = 0; partIx < PART_COUNT; partIx++) {
				if (stateSnapshot.partStates[partIx].polyStateChanged) {
					emit reportHandler.polyStateChanged(partIx);
					stateSnapshot.partStates[partIx].polyStateChanged = false;
				}

				if (stateSnapshot.partStates[partIx].programChanged) {
					emit reportHandler.programChanged(partIx, stateSnapshot.partStates[partIx].soundGroupName, stateSnapshot.partStates[partIx].timbreName);
					stateSnapshot.partStates[partIx].programChanged = false;
				}
			}

			emit qsynth.audioBlockRendered();
		}
	}

public:
	RealtimeHelper(QSynth &useQSynth) :
		qsynth(useQSynth),
		stopProcessing(),
		outputGain(qsynth.synth->getOutputGain()),
		reverbOutputGain(qsynth.synth->getReverbOutputGain()),
		reverbEnabled(!qsynth.synth->isReverbOverridden() || qsynth.synth->isReverbEnabled()),
		reverbOverridden(qsynth.synth->isReverbOverridden()),
		reversedStereoEnabled(qsynth.synth->isReversedStereoEnabled()),
		niceAmpRampEnabled(qsynth.synth->isNiceAmpRampEnabled()),
		nicePanningEnabled(qsynth.synth->isNicePanningEnabled()),
		nicePartialMixingEnabled(qsynth.synth->isNicePartialMixingEnabled()),
		emuDACInputMode(qsynth.synth->getDACInputMode()),
		midiDelayMode(qsynth.synth->getMIDIDelayMode()),
		tempState(),
		stateSnapshot()
	{
		tempState.masterVolumeUpdate = NO_UPDATE_VALUE;
		tempState.reverbMode = NO_UPDATE_VALUE;
		tempState.reverbTime = NO_UPDATE_VALUE;
		tempState.reverbLevel = NO_UPDATE_VALUE;
		stateSnapshot.midiMessageLEDState = qsynth.synth->getDisplayState(stateSnapshot.lcdState);
		for (int i = 0; i < PART_COUNT; i++) {
			partVolumeOverride[i] = NO_UPDATE_VALUE;
		}
		for (int i = 0; i < VOICE_PART_COUNT; i++) {
			partTimbres[i].changed = 0;
		}
		{
			makeSoundGroups(*qsynth.synth, soundGroupCache);
			char memoryGroupName[SOUND_GROUP_NAME_LENGTH];
			qsynth.synth->getSoundGroupName(memoryGroupName, 2, 0);
			SoundGroup soundGroup = {memoryGroupName, QVector<SoundGroup::Item>(64)};
			for (uint i = 0; i < 64; i++) {
				SoundGroup::Item item = {2, i, "Timbre #" + QString().setNum(i)};
				soundGroup.constituents[i] = item;
			}
			if (soundGroupCache.empty()) {
				soundGroupCache += soundGroup;
			} else {
				soundGroupCache.insert(soundGroupCache.size() - 1U, soundGroup);
			}
		}
	}

	~RealtimeHelper() {
		QMutexLocker stateSnapshotLocker(&stateSnapshotMutex);
		stopProcessing = true;
		renderCompleteCondition.wakeOne();
		stateSnapshotLocker.unlock();
		wait();
	}

	void getSynthSettings(SynthProfile &synthProfile) {
		QMutexLocker settingsLocker(&settingsMutex);
		synthProfile.outputGain = outputGain;
		synthProfile.reverbOutputGain = reverbOutputGain;
		synthProfile.reverbOverridden = reverbOverridden;
		synthProfile.reverbEnabled = reverbEnabled;
		synthProfile.reverbMode = qsynth.reverbMode;
		synthProfile.reverbTime = qsynth.reverbTime;
		synthProfile.reverbLevel = qsynth.reverbLevel;
		synthProfile.reversedStereoEnabled = reversedStereoEnabled;
		synthProfile.niceAmpRamp = niceAmpRampEnabled;
		synthProfile.nicePanning = nicePanningEnabled;
		synthProfile.nicePartialMixing = nicePartialMixingEnabled;
		synthProfile.emuDACInputMode = emuDACInputMode;
		synthProfile.midiDelayMode = midiDelayMode;
	}

	void setMasterVolume(int useMasterVolume) {
		QMutexLocker settingsLocker(&settingsMutex);
		masterVolume = useMasterVolume;
		enqueueSynthControlEvent(MASTER_VOLUME_CHANGED);
	}

	void setOutputGain(float useOutputGain) {
		QMutexLocker settingsLocker(&settingsMutex);
		outputGain = useOutputGain;
		enqueueSynthControlEvent(OUTPUT_GAIN_CHANGED);
	}

	void setReverbOutputGain(float useReverbOutputGain) {
		QMutexLocker settingsLocker(&settingsMutex);
		reverbOutputGain = useReverbOutputGain;
		enqueueSynthControlEvent(REVERB_OUTPUT_GAIN_CHANGED);
	}

	void setReverbEnabled(bool useReverbEnabled) {
		QMutexLocker settingsLocker(&settingsMutex);
		reverbEnabled = useReverbEnabled;
		enqueueSynthControlEvent(REVERB_ENABLED_CHANGED);
	}

	void setReverbOverridden(bool useReverbOverridden) {
		QMutexLocker settingsLocker(&settingsMutex);
		reverbOverridden = useReverbOverridden;
		enqueueSynthControlEvent(REVERB_OVERRIDDEN_CHANGED);
	}

	void setReverbSettings(int useReverbMode, int useReverbTime, int useReverbLevel) {
		QMutexLocker settingsLocker(&settingsMutex);
		qsynth.reverbMode = useReverbMode;
		qsynth.reverbTime = useReverbTime;
		qsynth.reverbLevel = useReverbLevel;
		enqueueSynthControlEvent(REVERB_SETTINGS_CHANGED);
	}

	void setPartVolumeOverride(uint partNumber, Bit8u volumeOverride) {
		QMutexLocker settingsLocker(&settingsMutex);
		partVolumeOverride[partNumber] = volumeOverride;
		enqueueSynthControlEvent(PART_VOLUME_OVERRIDE_CHANGED);
	}

	void setPartTimbre(uint partNumber, Bit8u timbreGroup, Bit8u timbreNumber) {
		QMutexLocker settingsLocker(&settingsMutex);
		partTimbres[partNumber].changed = 1;
		partTimbres[partNumber].group = timbreGroup;
		partTimbres[partNumber].number = timbreNumber;
		enqueueSynthControlEvent(PART_TIMBRE_CHANGED);
	}

	void setReversedStereoEnabled(bool useReversedStereoEnabled) {
		QMutexLocker settingsLocker(&settingsMutex);
		reversedStereoEnabled = useReversedStereoEnabled;
		enqueueSynthControlEvent(REVERSED_STEREO_ENABLED_CHANGED);
	}

	void setNiceAmpRampEnabled(bool useNiceAmpRampEnabled) {
		QMutexLocker settingsLocker(&settingsMutex);
		niceAmpRampEnabled = useNiceAmpRampEnabled;
		enqueueSynthControlEvent(NICE_AMP_RAMP_ENABLED_CHANGED);
	}

	void setNicePanningEnabled(bool useNicePanningEnabled) {
		QMutexLocker settingsLocker(&settingsMutex);
		nicePanningEnabled = useNicePanningEnabled;
		enqueueSynthControlEvent(NICE_PANNING_ENABLED_CHANGED);
	}

	void setNicePartialMixingEnabled(bool useNicePartialMixingEnabled) {
		QMutexLocker settingsLocker(&settingsMutex);
		nicePartialMixingEnabled = useNicePartialMixingEnabled;
		enqueueSynthControlEvent(NICE_PARTIAL_MIXING_ENABLED_CHANGED);
	}

	void setDACInputMode(DACInputMode useEmuDACInputMode) {
		QMutexLocker settingsLocker(&settingsMutex);
		emuDACInputMode = useEmuDACInputMode;
		enqueueSynthControlEvent(EMU_DAC_INPUT_MODE_CHANGED);
	}

	void setMIDIDelayMode(MIDIDelayMode useMIDIDelayMode) {
		QMutexLocker settingsLocker(&settingsMutex);
		midiDelayMode = useMIDIDelayMode;
		enqueueSynthControlEvent(MIDI_DELAY_MODE_CHANGED);
	}

	void resetMidiChannelsAssignment(bool useMidiChannelsAssignmentChannel1Engaged) {
		QMutexLocker settingsLocker(&settingsMutex);
		midiChannelsAssignmentChannel1Engaged = useMidiChannelsAssignmentChannel1Engaged;
		enqueueSynthControlEvent(MIDI_CHANNELS_ASSIGNMENT_RESET);
	}

	void setMainDisplayMode() {
		QMutexLocker settingsLocker(&settingsMutex);
		enqueueSynthControlEvent(DISPLAY_RESET);
	}

	void setDisplayCompatibilityMode(DisplayCompatibilityMode useDisplayCompatibilityMode) {
		QMutexLocker settingsLocker(&settingsMutex);
		displayCompatibilityMode = useDisplayCompatibilityMode;
		enqueueSynthControlEvent(DISPLAY_COMPATIBILITY_MODE_CHANGED);
	}

	void resetSynth() {
		QMutexLocker settingsLocker(&settingsMutex);
		enqueueSynthControlEvent(SYNTH_RESET);
	}

	bool playMIDIShortMessageRealtime(Bit32u msg, quint64 timestamp) const {
		RealtimeLocker midiLocker(*qsynth.midiMutex);
		return midiLocker.isLocked() && qsynth.isOpen() && qsynth.synth->playMsg(msg, qsynth.convertOutputToSynthTimestamp(timestamp));
	}

	bool playMIDISysexRealtime(const Bit8u *sysex, Bit32u sysexLen, quint64 timestamp) const {
		RealtimeLocker midiLocker(*qsynth.midiMutex);
		return midiLocker.isLocked() && qsynth.isOpen() && qsynth.synth->playSysex(sysex, sysexLen, qsynth.convertOutputToSynthTimestamp(timestamp));
	}

	void renderRealtime(float *buffer, uint length) {
		RealtimeLocker synthLocker(*qsynth.synthMutex);
		if (synthLocker.isLocked() && qsynth.isOpen()) {
			applyChangesRealtime();
			qsynth.sampleRateConverter->getOutputSamples(buffer, length);
			saveStateRealtime();
			renderCompleteCondition.wakeOne();
		} else {
			Synth::muteSampleBuffer(buffer, length);
		}
	}

	void getPartialStates(PartialState *partialStates) {
		QMutexLocker stateSnapshotLocker(&stateSnapshotMutex);
		if (!qsynth.isOpen()) return;
		memcpy(partialStates, stateSnapshot.partialStates, qsynth.synth->getPartialCount() * sizeof(PartialState));
	}

	uint getPlayingNotes(uint partNumber, Bit8u *keys, Bit8u *velocities) {
		QMutexLocker stateSnapshotLocker(&stateSnapshotMutex);
		if (!qsynth.isOpen()) return 0;
		Bit32u playingNotesCount = stateSnapshot.partStates[partNumber].playingNotesCount;
		memcpy(keys, stateSnapshot.partStates[partNumber].keysOfPlayingNotes, playingNotesCount * sizeof(Bit8u));
		memcpy(velocities, stateSnapshot.partStates[partNumber].velocitiesOfPlayingNotes, playingNotesCount * sizeof(Bit8u));
		return playingNotesCount;
	}

	bool getDisplayState(char *targetBuffer) {
		QMutexLocker stateSnapshotLocker(&stateSnapshotMutex);
		memcpy(targetBuffer, stateSnapshot.lcdState, LCD_MESSAGE_LENGTH);
		return stateSnapshot.midiMessageLEDState;
	}

	const QVector<SoundGroup> &getSoundGroupCache() const {
		return soundGroupCache;
	}

	void onLCDMessage(const char *message) {
		memcpy(tempState.lcdMessage, message, LCD_MESSAGE_LENGTH - 1);
	}

	void onMasterVolumeChanged(Bit8u useMasterVolume) {
		tempState.masterVolumeUpdate = useMasterVolume;
	}

	void onReverbModeUpdated(Bit8u mode) {
		tempState.reverbMode = mode;
	}

	void onReverbTimeUpdated(Bit8u time) {
		tempState.reverbTime = time;
	}

	void onReverbLevelUpdated(Bit8u level) {
		tempState.reverbLevel = level;
	}

	void onPolyStateChanged(Bit8u partNum) {
		tempState.partStates[partNum].polyStateChanged = true;
	}

	void onProgramChanged(Bit8u partNum, const char soundGroupName[], const char patchName[]) {
		tempState.partStates[partNum].programChanged = true;
		memcpy(tempState.partStates[partNum].soundGroupName, soundGroupName, SOUND_GROUP_NAME_LENGTH - 1);
		memcpy(tempState.partStates[partNum].timbreName, patchName, TIMBRE_NAME_LENGTH - 1);
	}

	void onLCDStateUpdated() {
		tempState.lcdStateUpdated = true;
	}

	void onMidiMessageLEDStateUpdated(bool ledState) {
		tempState.midiMessageLEDState = ledState;
		tempState.midiMessageLEDStateUpdated = true;
	}
};

QReportHandler::QReportHandler(QSynth *qsynth) : QObject(qsynth) {
	connect(this, SIGNAL(balloonMessageAppeared(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

void QReportHandler::printDebug(const char *fmt, va_list list) {
	if (qSynth()->isRealtime()) return;
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
	qDebug() << "MT32:" << QString().vsprintf(fmt, list);
#else
	qDebug() << "MT32:" << QString().vasprintf(fmt, list);
#endif
}

void QReportHandler::showLCDMessage(const char *message) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onLCDMessage(message);
	} else {
		doShowLCDMessage(message);
	}
}

void QReportHandler::onErrorControlROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "Control ROM file cannot be opened.");
}

void QReportHandler::onErrorPCMROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "PCM ROM file cannot be opened.");
}

void QReportHandler::onDeviceReconfig() {
	int masterVolume = readMasterVolume(qSynth()->synth);
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onMasterVolumeChanged(masterVolume);
	} else {
		emit masterVolumeChanged(masterVolume);
	}
}

void QReportHandler::onDeviceReset() {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onMasterVolumeChanged(INITIAL_MASTER_VOLUME);
	} else {
		emit masterVolumeChanged(INITIAL_MASTER_VOLUME);
	}
}

void QReportHandler::onNewReverbMode(Bit8u mode) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onReverbModeUpdated(mode);
	} else {
		qSynth()->reverbMode = mode;
		emit reverbModeChanged(mode);
	}
}

void QReportHandler::onNewReverbTime(Bit8u time) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onReverbTimeUpdated(time);
	} else {
		qSynth()->reverbTime = time;
		emit reverbTimeChanged(time);
	}
}

void QReportHandler::onNewReverbLevel(Bit8u level) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onReverbLevelUpdated(level);
	} else {
		qSynth()->reverbLevel = level;
		emit reverbLevelChanged(level);
	}
}

void QReportHandler::onPolyStateChanged(Bit8u partNum) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onPolyStateChanged(partNum);
	} else {
		emit polyStateChanged(partNum);
	}
}

void QReportHandler::onProgramChanged(Bit8u partNum, const char soundGroupName[], const char patchName[]) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onProgramChanged(partNum, soundGroupName, patchName);
	} else {
		emit programChanged(partNum, QString().fromLocal8Bit(soundGroupName), QString().fromLocal8Bit(patchName));
	}
}

void QReportHandler::onLCDStateUpdated() {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onLCDStateUpdated();
	} else {
		emit lcdStateChanged();
	}
}

void QReportHandler::onMidiMessageLEDStateUpdated(bool ledState) {
	if (qSynth()->isRealtime()) {
		qSynth()->realtimeHelper->onMidiMessageLEDStateUpdated(ledState);
	} else {
		emit midiMessageLEDStateChanged(ledState);
	}
}

void QReportHandler::doShowLCDMessage(const char *message) {
	qDebug() << "LCD-Message:" << message;
	if (Master::getInstance()->getSettings()->value("Master/showLCDBalloons", true).toBool()) {
		emit balloonMessageAppeared("LCD-Message:", message);
	}
}

QSynth::QSynth(QObject *parent) :
	QObject(parent), state(SynthState_CLOSED), midiMutex(new QMutex), synthMutex(new QMutex),
	controlROMImage(), pcmROMImage(), synth(), reportHandler(this), sampleRateConverter(),
	audioRecorder(), realtimeHelper()
{
	createSynth();
}

QSynth::~QSynth() {
	freeROMImages();
	delete realtimeHelper;
	delete audioRecorder;
	delete sampleRateConverter;
	delete synth;
	delete synthMutex;
	delete midiMutex;
}

void QSynth::createSynth() {
	delete synth;
	synth = new Synth;
	synth->setReportHandler2(&reportHandler);
}

bool QSynth::isOpen() const {
	return state == SynthState_OPEN;
}

void QSynth::flushMIDIQueue() const {
	QMutexLocker midiLocker(midiMutex);
	QMutexLocker synthLocker(synthMutex);
	synth->flushMIDIQueue();
}

void QSynth::playMIDIShortMessageNow(Bit32u msg) const {
	QMutexLocker synthLocker(synthMutex);
	if (isOpen()) synth->playMsgNow(msg);
}

void QSynth::playMIDISysexNow(const Bit8u *sysex, Bit32u sysexLen) const {
	QMutexLocker synthLocker(synthMutex);
	if (isOpen()) synth->playSysexNow(sysex, sysexLen);
}

bool QSynth::playMIDIShortMessage(Bit32u msg, quint64 timestamp) const {
	if (isRealtime()) {
		return realtimeHelper->playMIDIShortMessageRealtime(msg, timestamp);
	} else {
		QMutexLocker midiLocker(midiMutex);
		return isOpen() && synth->playMsg(msg, convertOutputToSynthTimestamp(timestamp));
	}
}

bool QSynth::playMIDISysex(const Bit8u *sysex, Bit32u sysexLen, quint64 timestamp) const {
	if (isRealtime()) {
		return realtimeHelper->playMIDISysexRealtime(sysex, sysexLen, timestamp);
	} else {
		QMutexLocker midiLocker(midiMutex);
		return isOpen() && synth->playSysex(sysex, sysexLen, convertOutputToSynthTimestamp(timestamp));
	}
}

Bit32u QSynth::convertOutputToSynthTimestamp(quint64 timestamp) const {
	return Bit32u(sampleRateConverter->convertOutputToSynthTimestamp(timestamp));
}

void QSynth::render(Bit16s *buffer, uint length) {
	QMutexLocker synthLocker(synthMutex);
	if (!isOpen()) {
		synthLocker.unlock();

		// Synth is closed, simply erase buffer content
		Synth::muteSampleBuffer(buffer, 2 * length);
		emit audioBlockRendered();
		return;
	}
	sampleRateConverter->getOutputSamples(buffer, length);
	if (isRecordingAudio()) {
		if (!audioRecorder->write(buffer, length)) stopRecordingAudio();
	}
	synthLocker.unlock();
	emit audioBlockRendered();
}

void QSynth::render(float *buffer, uint length) {
	if (isRealtime()) {
		realtimeHelper->renderRealtime(buffer, length);
		return;
	}
	QMutexLocker synthLocker(synthMutex);
	if (!isOpen()) {
		synthLocker.unlock();

		// Synth is closed, simply erase buffer content
		Synth::muteSampleBuffer(buffer, 2 * length);
		emit audioBlockRendered();
		return;
	}
	sampleRateConverter->getOutputSamples(buffer, length);
	synthLocker.unlock();
	// TODO: Add support for recording to float WAVs
	emit audioBlockRendered();
}

bool QSynth::open(uint &targetSampleRate, SamplerateConversionQuality srcQuality, const QString useSynthProfileName) {
	if (isOpen()) return true;

	if (!useSynthProfileName.isEmpty()) synthProfileName = useSynthProfileName;
	SynthProfile synthProfile;

	forever {
		Master::getInstance()->loadSynthProfile(synthProfile, synthProfileName);
		if (controlROMImage == NULL || pcmROMImage == NULL) {
			Master::getInstance()->findROMImages(synthProfile, controlROMImage, pcmROMImage);
		}
		if (controlROMImage == NULL) {
			controlROMImage = makeROMImage(synthProfile.romDir, synthProfile.controlROMFileName, synthProfile.controlROMFileName2);
		}
		if (controlROMImage != NULL && pcmROMImage == NULL) {
			pcmROMImage = makeROMImage(synthProfile.romDir, synthProfile.pcmROMFileName, synthProfile.pcmROMFileName2);
		}
		if (controlROMImage != NULL && pcmROMImage != NULL) break;
		qDebug() << "Missing ROM files. Can't open synth :(";
		freeROMImages();
		if (!Master::getInstance()->handleROMSLoadFailed(synthProfileName)) return false;
	}

	AnalogOutputMode actualAnalogOutputMode = synthProfile.analogOutputMode;
	const AnalogOutputMode bestAnalogOutputMode = SampleRateConverter::getBestAnalogOutputMode(targetSampleRate);
	if (actualAnalogOutputMode == AnalogOutputMode_ACCURATE && bestAnalogOutputMode == AnalogOutputMode_OVERSAMPLED) {
		actualAnalogOutputMode = bestAnalogOutputMode;
	}
	setRendererType(synthProfile.rendererType);

	static const char *ANALOG_OUTPUT_MODES[] = {"Digital only", "Coarse", "Accurate", "Oversampled2x"};
	qDebug() << "Using Analogue output mode:" << ANALOG_OUTPUT_MODES[actualAnalogOutputMode];
	qDebug() << "Using Renderer Type:" << (synthProfile.rendererType ? "Float 32-bit" : "Integer 16-bit");
	qDebug() << "Using Max Partials:" << synthProfile.partialCount;

	targetSampleRate = SampleRateConverter::getSupportedOutputSampleRate(targetSampleRate);

	if (synth->open(*controlROMImage, *pcmROMImage, Bit32u(synthProfile.partialCount), actualAnalogOutputMode)) {
		setState(SynthState_OPEN);
		reportHandler.onDeviceReconfig();
		setSynthProfile(synthProfile, synthProfileName);
		if (engageChannel1OnOpen) resetMIDIChannelsAssignment(true);
		if (targetSampleRate == 0) targetSampleRate = getSynthSampleRate();
		sampleRateConverter = new SampleRateConverter(*synth, targetSampleRate, srcQuality);
		return true;
	}
	createSynth();
	setState(SynthState_CLOSED);
	freeROMImages();
	return false;
}

void QSynth::setMasterVolume(int masterVolume) {
	if (isRealtime()) {
		realtimeHelper->setMasterVolume(masterVolume);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) writeMasterVolumeSysex(synth, masterVolume);
	}
}

void QSynth::setOutputGain(float outputGain) {
	if (isRealtime()) {
		realtimeHelper->setOutputGain(outputGain);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setOutputGain(outputGain);
	}
}

void QSynth::setReverbOutputGain(float reverbOutputGain) {
	if (isRealtime()) {
		realtimeHelper->setReverbOutputGain(reverbOutputGain);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setReverbOutputGain(reverbOutputGain);
	}
}

void QSynth::setReverbEnabled(bool reverbEnabled) {
	if (isRealtime()) {
		realtimeHelper->setReverbEnabled(reverbEnabled);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setReverbEnabled(reverbEnabled);
	}
}

void QSynth::setReverbOverridden(bool reverbOverridden) {
	if (isRealtime()) {
		realtimeHelper->setReverbOverridden(reverbOverridden);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setReverbOverridden(reverbOverridden);
	}
}

void QSynth::setReverbSettings(int useReverbMode, int useReverbTime, int useReverbLevel) {
	if (isRealtime()) {
		realtimeHelper->setReverbSettings(useReverbMode, useReverbTime, useReverbLevel);
	} else {
		QMutexLocker synthLocker(synthMutex);
		reverbMode = useReverbMode;
		reverbTime = useReverbTime;
		reverbLevel = useReverbLevel;
		if (isOpen()) overrideReverbSettings(synth, useReverbMode, useReverbTime, useReverbLevel);
	}
}

void QSynth::setPartVolumeOverride(uint partNumber, uint volumeOverride) {
	if (isRealtime()) {
		realtimeHelper->setPartVolumeOverride(partNumber, volumeOverride);
	} else {
		QMutexLocker synthLocker(synthMutex);
		synth->setPartVolumeOverride(Bit8u(partNumber), Bit8u(volumeOverride));
	}
}

void QSynth::setReversedStereoEnabled(bool enabled) {
	if (isRealtime()) {
		realtimeHelper->setReversedStereoEnabled(enabled);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setReversedStereoEnabled(enabled);
	}
}

void QSynth::setNiceAmpRampEnabled(bool enabled) {
	if (isRealtime()) {
		realtimeHelper->setNiceAmpRampEnabled(enabled);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setNiceAmpRampEnabled(enabled);
	}
}

void QSynth::setNicePanningEnabled(bool enabled) {
	if (isRealtime()) {
		realtimeHelper->setNicePanningEnabled(enabled);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setNicePanningEnabled(enabled);
	}
}

void QSynth::setNicePartialMixingEnabled(bool enabled) {
	if (isRealtime()) {
		realtimeHelper->setNicePartialMixingEnabled(enabled);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setNicePartialMixingEnabled(enabled);
	}
}

void QSynth::resetMIDIChannelsAssignment(bool engageChannel1) {
	if (isRealtime()) {
		realtimeHelper->resetMidiChannelsAssignment(engageChannel1);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) writeMIDIChannelsAssignmentResetSysex(synth, engageChannel1);
	}
}

void QSynth::setInitialMIDIChannelsAssignment(bool engageChannel1) {
	engageChannel1OnOpen = engageChannel1;
}

void QSynth::setReverbCompatibilityMode(ReverbCompatibilityMode useReverbCompatibilityMode) {
	reverbCompatibilityMode = useReverbCompatibilityMode;
	QMutexLocker synthLocker(synthMutex);
	if (!isOpen()) return;
	bool mt32CompatibleReverb;
	if (useReverbCompatibilityMode == ReverbCompatibilityMode_DEFAULT) {
		mt32CompatibleReverb = synth->isDefaultReverbMT32Compatible();
	} else {
		mt32CompatibleReverb = useReverbCompatibilityMode == ReverbCompatibilityMode_MT32;
	}
	synth->setReverbCompatibilityMode(mt32CompatibleReverb);
}

void QSynth::setMIDIDelayMode(MIDIDelayMode midiDelayMode) {
	if (isRealtime()) {
		realtimeHelper->setMIDIDelayMode(midiDelayMode);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setMIDIDelayMode(midiDelayMode);
	}
}

void QSynth::setDACInputMode(DACInputMode emuDACInputMode) {
	if (isRealtime()) {
		realtimeHelper->setDACInputMode(emuDACInputMode);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) synth->setDACInputMode(emuDACInputMode);
	}
}

void QSynth::setAnalogOutputMode(AnalogOutputMode useAnalogOutputMode) {
	analogOutputMode = useAnalogOutputMode;
}

void QSynth::setRendererType(RendererType useRendererType) {
	synth->selectRendererType(useRendererType);
}

void QSynth::setPartialCount(int newPartialCount) {
	partialCount = qBound(MIN_PARTIAL_COUNT, newPartialCount, MAX_PARTIAL_COUNT);
}

const QString QSynth::getPatchName(int partNum) const {
	QMutexLocker synthLocker(synthMutex);
	QString name = isOpen() ? QString().fromLocal8Bit(synth->getPatchName(partNum)) : QString("Channel %1").arg(partNum + 1);
	return name;
}

void QSynth::setTimbreOnPart(uint partNumber, uint timbreGroup, uint timbreNumber) {
	if (isRealtime()) {
		realtimeHelper->setPartTimbre(partNumber, timbreGroup, timbreNumber);
		return;
	}
	QMutexLocker synthLocker(synthMutex);
	writeTimbreSelectionOnPartSysex(synth, partNumber, timbreGroup, timbreNumber);
}

void QSynth::getSoundGroups(QVector<SoundGroup> &groups) const {
	if (isRealtime()) {
		groups << realtimeHelper->getSoundGroupCache();
		return;
	}
	QMutexLocker synthLocker(synthMutex);
	makeSoundGroups(*synth, groups);
}

void QSynth::getPartialStates(PartialState *partialStates) const {
	if (isRealtime()) {
		realtimeHelper->getPartialStates(partialStates);
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (!isOpen()) return;
		synth->getPartialStates(partialStates);
	}
}

uint QSynth::getPlayingNotes(uint partNumber, Bit8u *keys, Bit8u *velocities) const {
	if (isRealtime()) {
		return realtimeHelper->getPlayingNotes(partNumber, keys, velocities);
	} else {
		QMutexLocker synthLocker(synthMutex);
		return synth->getPlayingNotes(partNumber, keys, velocities);
	}
}

uint QSynth::getPartialCount() const {
	return synth->getPartialCount();
}

uint QSynth::getSynthSampleRate() const {
	return synth->getStereoOutputSampleRate();
}

bool QSynth::isActive() const {
	QMutexLocker synthLocker(synthMutex);
	return isOpen() && synth->isActive();
}

bool QSynth::getDisplayState(char *targetBuffer) const {
	if (isRealtime()) {
		return realtimeHelper->getDisplayState(targetBuffer);
	}
	QMutexLocker synthLocker(synthMutex);
	return synth->getDisplayState(targetBuffer);
}

void QSynth::setMainDisplayMode() {
	if (isRealtime()) {
		realtimeHelper->setMainDisplayMode();
	} else {
		QMutexLocker synthLocker(synthMutex);
		synth->setMainDisplayMode();
	}
}

void QSynth::setDisplayCompatibilityMode(DisplayCompatibilityMode useDisplayCompatibilityMode) {
	if (isRealtime()) {
		realtimeHelper->setDisplayCompatibilityMode(useDisplayCompatibilityMode);
	} else {
		QMutexLocker synthLocker(synthMutex);
		displayCompatibilityMode = useDisplayCompatibilityMode;
		if (DisplayCompatibilityMode_DEFAULT == displayCompatibilityMode) {
			synth->setDisplayCompatibility(synth->isDefaultDisplayOldMT32Compatible());
		} else {
			synth->setDisplayCompatibility(DisplayCompatibilityMode_OLD_MT32 == displayCompatibilityMode);
		}
	}
}

void QSynth::reset() const {
	if (isRealtime()) {
		realtimeHelper->resetSynth();
	} else {
		QMutexLocker synthLocker(synthMutex);
		if (isOpen()) writeSystemResetSysex(synth);
	}
}

bool QSynth::isRealtime() const {
	return realtimeHelper != NULL;
}

void QSynth::enableRealtime() {
	QMutexLocker synthLocker(synthMutex);
	synth->preallocateReverbMemory(true);
	synth->configureMIDIEventQueueSysexStorage(MAX_STREAM_BUFFER_SIZE);
	if (isRealtime()) return;
	realtimeHelper = new RealtimeHelper(*this);
	realtimeHelper->start();
	qDebug() << "QSynth: Realtime rendering initialised";
}

void QSynth::setState(SynthState newState) {
	if (state == newState) return;
	state = newState;
	emit stateChanged(newState);
}

void QSynth::close() {
	if (!isOpen()) return;
	setState(SynthState_CLOSING);
	{
		QMutexLocker midiLocker(midiMutex);
		QMutexLocker synthLocker(synthMutex);
		synth->close();
		// This effectively resets rendered frame counter, audioStream is also going down
		createSynth();
		delete sampleRateConverter;
		sampleRateConverter = NULL;
	}
	setState(SynthState_CLOSED);
	freeROMImages();
}

void QSynth::getSynthProfile(SynthProfile &synthProfile) const {
	synthProfile.romDir = romDir;
	synthProfile.controlROMFileName = controlROMFileName;
	synthProfile.controlROMFileName2 = controlROMFileName2;
	synthProfile.pcmROMFileName = pcmROMFileName;
	synthProfile.pcmROMFileName2 = pcmROMFileName2;
	synthProfile.analogOutputMode = analogOutputMode;
	synthProfile.rendererType = synth->getSelectedRendererType();
	synthProfile.partialCount = partialCount;
	synthProfile.engageChannel1OnOpen = engageChannel1OnOpen;
	synthProfile.reverbCompatibilityMode = reverbCompatibilityMode;
	synthProfile.displayCompatibilityMode = displayCompatibilityMode;

	if (isRealtime()) {
		realtimeHelper->getSynthSettings(synthProfile);
	} else {
		QMutexLocker locker(synthMutex);
		synthProfile.emuDACInputMode = synth->getDACInputMode();
		synthProfile.midiDelayMode = synth->getMIDIDelayMode();
		synthProfile.outputGain = synth->getOutputGain();
		synthProfile.reverbOutputGain = synth->getReverbOutputGain();
		synthProfile.reverbOverridden = synth->isReverbOverridden();
		synthProfile.reverbEnabled = !synthProfile.reverbOverridden || synth->isReverbEnabled();
		synthProfile.reverbMode = reverbMode;
		synthProfile.reverbTime = reverbTime;
		synthProfile.reverbLevel = reverbLevel;
		synthProfile.reversedStereoEnabled = synth->isReversedStereoEnabled();
		synthProfile.niceAmpRamp = synth->isNiceAmpRampEnabled();
		synthProfile.nicePanning = synth->isNicePanningEnabled();
		synthProfile.nicePartialMixing = synth->isNicePartialMixingEnabled();
	}
}

void QSynth::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	synthProfileName = useSynthProfileName;

	// Settings below do not take effect before re-open.
	romDir = synthProfile.romDir;
	controlROMFileName = synthProfile.controlROMFileName;
	controlROMFileName2 = synthProfile.controlROMFileName2;
	pcmROMFileName = synthProfile.pcmROMFileName;
	pcmROMFileName2 = synthProfile.pcmROMFileName2;
	setAnalogOutputMode(synthProfile.analogOutputMode);
	setRendererType(synthProfile.rendererType);
	setPartialCount(synthProfile.partialCount);

	// Settings below take effect immediately.
	setReverbCompatibilityMode(synthProfile.reverbCompatibilityMode);
	setMIDIDelayMode(synthProfile.midiDelayMode);
	setDACInputMode(synthProfile.emuDACInputMode);
	setOutputGain(synthProfile.outputGain);
	setReverbOutputGain(synthProfile.reverbOutputGain);
	setReverbOverridden(synthProfile.reverbOverridden);
	if (synthProfile.reverbOverridden) {
		setReverbSettings(synthProfile.reverbMode, synthProfile.reverbTime, synthProfile.reverbLevel);
		setReverbEnabled(synthProfile.reverbEnabled);
	}
	setReversedStereoEnabled(synthProfile.reversedStereoEnabled);
	setNiceAmpRampEnabled(synthProfile.niceAmpRamp);
	setNicePanningEnabled(synthProfile.nicePanning);
	setNicePartialMixingEnabled(synthProfile.nicePartialMixing);
	setInitialMIDIChannelsAssignment(synthProfile.engageChannel1OnOpen);
	setDisplayCompatibilityMode(synthProfile.displayCompatibilityMode);
}

void QSynth::getROMImages(const ROMImage *&cri, const ROMImage *&pri) const {
	cri = controlROMImage;
	pri = pcmROMImage;
}

void QSynth::freeROMImages() {
	// Ensure our ROM images get freed even if the synth is still in use
	const ROMImage *cri = controlROMImage;
	controlROMImage = NULL;
	const ROMImage *pri = pcmROMImage;
	pcmROMImage = NULL;
	Master::getInstance()->freeROMImages(cri, pri);
}

const QReportHandler *QSynth::getReportHandler() const {
	return &reportHandler;
}

void QSynth::startRecordingAudio(const QString &fileName) {
	QMutexLocker synthLocker(synthMutex);
	if (isRecordingAudio()) delete audioRecorder;
	audioRecorder = new AudioFileWriter(sampleRateConverter->convertSynthToOutputTimestamp(SAMPLE_RATE), fileName);
	audioRecorder->open();
}

void QSynth::stopRecordingAudio() {
	QMutexLocker synthLocker(synthMutex);
	if (!isRecordingAudio()) return;
	audioRecorder->close();
	delete audioRecorder;
	audioRecorder = NULL;
}

bool QSynth::isRecordingAudio() const {
	return audioRecorder != NULL;
}
