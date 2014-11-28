/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

/**
 * This is a wrapper for MT32Emu::Synth that adds thread-safe queueing of MIDI messages.
 *
 * Thread safety:
 * pushMIDIShortMessage() and pushMIDISysex() can be called by any number of threads safely.
 * All other functions may only be called by a single thread.
 */

#include <QtGlobal>
#include <QMessageBox>

#include "QSynth.h"
#include "Master.h"
#include "MasterClock.h"
#include "resample/SampleRateConverter.h"

using namespace MT32Emu;

QReportHandler::QReportHandler(QObject *parent) : QObject(parent) {
	connect(this, SIGNAL(balloonMessageAppeared(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

void QReportHandler::showLCDMessage(const char *message) {
	qDebug() << "LCD-Message:" << message;
	if (Master::getInstance()->getSettings()->value("Master/showLCDBalloons", true).toBool()) {
		emit balloonMessageAppeared("LCD-Message:", message);
	}
	emit lcdMessageDisplayed(message);
}

void QReportHandler::onErrorControlROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "Control ROM file cannot be opened.");
}

void QReportHandler::onErrorPCMROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "PCM ROM file cannot be opened.");
}

void QReportHandler::onMIDIMessagePlayed() {
	emit midiMessagePlayed();
}

void QReportHandler::onDeviceReconfig() {
	QSynth *qsynth = (QSynth *)parent();
	Bit8u currentMasterVolume = 0;
	qsynth->synth->readMemory(0x40016, 1, &currentMasterVolume);
	int masterVolume = currentMasterVolume;
	emit masterVolumeChanged(masterVolume);
}

void QReportHandler::onDeviceReset() {
	emit masterVolumeChanged(100);
}

void QReportHandler::onNewReverbMode(Bit8u mode) {
	((QSynth *)parent())->reverbMode = mode;
	emit reverbModeChanged(mode);
}

void QReportHandler::onNewReverbTime(Bit8u time) {
	((QSynth *)parent())->reverbTime = time;
	emit reverbTimeChanged(time);
}

void QReportHandler::onNewReverbLevel(Bit8u level) {
	((QSynth *)parent())->reverbLevel = level;
	emit reverbLevelChanged(level);
}

void QReportHandler::onPolyStateChanged(int partNum) {
	emit polyStateChanged(partNum);
}

void QReportHandler::onProgramChanged(int partNum, int timbreGroup, const char patchName[]) {
	emit programChanged(partNum, timbreGroup, QString().fromAscii(patchName));
}

QSynth::QSynth(QObject *parent) :
	QObject(parent), state(SynthState_CLOSED), midiMutex(QMutex::Recursive),
	controlROMImage(NULL), pcmROMImage(NULL), reportHandler(this), sampleRateConverter(NULL)
{
	synthMutex = new QMutex(QMutex::Recursive);
	synth = new Synth(&reportHandler);
}

QSynth::~QSynth() {
	freeROMImages();
	if (sampleRateConverter != NULL) delete sampleRateConverter;
	delete synth;
	delete synthMutex;
}

bool QSynth::isOpen() const {
	return state == SynthState_OPEN;
}

void QSynth::flushMIDIQueue() {
	midiMutex.lock();
	synthMutex->lock();
	// Drain synth's queue first
	synth->flushMIDIQueue();
	synthMutex->unlock();
	midiMutex.unlock();
}

void QSynth::playMIDIShortMessageNow(Bit32u msg) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->playMsgNow(msg);
	synthMutex->unlock();
}

void QSynth::playMIDISysexNow(const Bit8u *sysex, Bit32u sysexLen) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->playSysexNow(sysex, sysexLen);
	synthMutex->unlock();
}

bool QSynth::playMIDIShortMessage(Bit32u msg, quint64 timestamp) {
	midiMutex.lock();
	if (!isOpen()) {
		midiMutex.unlock();
		return false;
	}
	bool eventPushed = synth->playMsg(msg, convertOutputToSynthTimestamp(timestamp));
	midiMutex.unlock();
	return eventPushed;
}

bool QSynth::playMIDISysex(const Bit8u *sysex, Bit32u sysexLen, quint64 timestamp) {
	midiMutex.lock();
	if (!isOpen()) {
		midiMutex.unlock();
		return false;
	}
	bool eventPushed = synth->playSysex(sysex, sysexLen, convertOutputToSynthTimestamp(timestamp));
	midiMutex.unlock();
	return eventPushed;
}

Bit32u QSynth::convertOutputToSynthTimestamp(quint64 timestamp) {
	return Bit32u(timestamp * sampleRateRatio);
}

void QSynth::render(Bit16s *buffer, uint length) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();

		// Synth is closed, simply erase buffer content
		memset(buffer, 0, length << 2);
		return;
	}
#if MT32EMU_USE_FLOAT_SAMPLES
	float fBuf[2 * MAX_SAMPLES_PER_RUN];
	while (0 < length) {
		uint framesToRender = qMin(length, MAX_SAMPLES_PER_RUN);
		if (sampleRateConverter != NULL) {
			sampleRateConverter->getOutputSamples(fBuf, framesToRender);
		} else {
			synth->render(fBuf, framesToRender);
		}
		for (uint i = 0; i < 2 * framesToRender; i++) {
			*(buffer++) = (Bit16s)qBound(-32768.0f, fBuf[i] * 16384.0f, 32767.0f);
		}
		length -= framesToRender;
	}
#else
	if (sampleRateConverter != NULL) {
		sampleRateConverter->getOutputSamples(buffer, length);
	} else {
		synth->render(buffer, length);
	}
#endif
	synthMutex->unlock();
}

bool QSynth::open(uint targetSampleRate, const QString useSynthProfileName) {
	if (isOpen()) {
		return true;
	}

	synthProfileName = useSynthProfileName;
	SynthProfile synthProfile;
	getSynthProfile(synthProfile);
	Master::getInstance()->loadSynthProfile(synthProfile, synthProfileName);
	setSynthProfile(synthProfile, synthProfileName);
	if (controlROMImage == NULL || pcmROMImage == NULL) {
		qDebug() << "Missing ROM files. Can't open synth :(";
		freeROMImages();
		return false;
	}
	actualAnalogOutputMode = synthProfile.analogOutputMode;
	static const char *ANALOG_OUTPUT_MODES[] = {"Digital only", "Coarse", "Accurate"};
	qDebug() << "Using Analogue output mode:" << ANALOG_OUTPUT_MODES[actualAnalogOutputMode];
	if (synth->open(*controlROMImage, *pcmROMImage, actualAnalogOutputMode)) {
		setState(SynthState_OPEN);
		reportHandler.onDeviceReconfig();
		setSynthProfile(synthProfile, synthProfileName);
		if (targetSampleRate > 0 && targetSampleRate != getSynthSampleRate()) {
			sampleRateConverter = SampleRateConverter::createSampleRateConverter(synth, targetSampleRate);
			sampleRateRatio = SAMPLE_RATE / (double)targetSampleRate;
		} else {
			sampleRateRatio = SAMPLE_RATE / (double)getSynthSampleRate();
		}
		return true;
	}
	// We're now in a partially-open state - better to properly close.
	synth->close(true);
	delete synth;
	synth = new Synth(&reportHandler);
	return false;
}

void QSynth::setMasterVolume(int masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, (Bit8u)masterVolume};

	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->writeSysex(16, sysex, 4);
	synthMutex->unlock();
}

void QSynth::setOutputGain(float outputGain) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setOutputGain(outputGain);
	synthMutex->unlock();
}

void QSynth::setReverbOutputGain(float reverbOutputGain) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOutputGain(reverbOutputGain);
	synthMutex->unlock();
}

void QSynth::setReverbEnabled(bool reverbEnabled) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbEnabled(reverbEnabled);
	synthMutex->unlock();
}

void QSynth::setReverbOverridden(bool reverbOverridden) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOverridden(reverbOverridden);
	synthMutex->unlock();
}

void QSynth::setReverbSettings(int reverbMode, int reverbTime, int reverbLevel) {
	this->reverbMode = reverbMode;
	this->reverbTime = reverbTime;
	this->reverbLevel = reverbLevel;
	Bit8u sysex[] = {0x10, 0x00, 0x01, (Bit8u)reverbMode, (Bit8u)reverbTime, (Bit8u)reverbLevel};

	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOverridden(false);
	synth->writeSysex(16, sysex, 6);
	synth->setReverbOverridden(true);
	synthMutex->unlock();
}

void QSynth::setReversedStereoEnabled(bool enabled) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReversedStereoEnabled(enabled);
	synthMutex->unlock();
}

void QSynth::resetMIDIChannelsAssignment(bool engageChannel1) {
	static const Bit8u sysexStandardChannelAssignment[] = {0x10, 0x00, 0x0d, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
	static const Bit8u sysexChannel1EngagedAssignment[] = {0x10, 0x00, 0x0d, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->writeSysex(16, engageChannel1 ? sysexChannel1EngagedAssignment : sysexStandardChannelAssignment, sizeof(sysexStandardChannelAssignment));
	synthMutex->unlock();
}

void QSynth::setReverbCompatibilityMode(ReverbCompatibilityMode useReverbCompatibilityMode) {
	reverbCompatibilityMode = useReverbCompatibilityMode;
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	bool mt32CompatibleReverb;
	if (useReverbCompatibilityMode == ReverbCompatibilityMode_DEFAULT) {
		mt32CompatibleReverb = controlROMImage->getROMInfo()->controlROMFeatures->isDefaultReverbMT32Compatible();
	} else {
		mt32CompatibleReverb = useReverbCompatibilityMode == ReverbCompatibilityMode_MT32;
	}
	synth->setReverbCompatibilityMode(mt32CompatibleReverb);
	synthMutex->unlock();
}

void QSynth::setMIDIDelayMode(MIDIDelayMode midiDelayMode) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setMIDIDelayMode(midiDelayMode);
	synthMutex->unlock();
}

void QSynth::setDACInputMode(DACInputMode emuDACInputMode) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setDACInputMode(emuDACInputMode);
	synthMutex->unlock();
}

void QSynth::setAnalogOutputMode(MT32Emu::AnalogOutputMode useAnalogOutputMode) {
	analogOutputMode = useAnalogOutputMode;
}

const QString QSynth::getPatchName(int partNum) const {
	synthMutex->lock();
	if (isOpen()) {
		const char *name = synth->getPart(partNum)->getCurrentInstr();
		synthMutex->unlock();
		return QString().fromAscii(name);
	}
	synthMutex->unlock();
	return QString("Channel %1").arg(partNum + 1);
}

const Partial *QSynth::getPartial(int partialNum) const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return NULL;
	}
	const Partial *partial = synth->getPartial(partialNum);
	synthMutex->unlock();
	return partial;
}

const Poly *QSynth::getFirstActivePolyOnPart(unsigned int partNum) const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return NULL;
	}
	const Poly *poly = synth->getPart(partNum)->getFirstActivePoly();
	synthMutex->unlock();
	return poly;
}

unsigned int QSynth::getPartialCount() const {
	synthMutex->lock();
	unsigned int partialCount = synth->getPartialCount();
	synthMutex->unlock();
	return partialCount;
}

unsigned int QSynth::getSynthSampleRate() const {
	return synth->getStereoOutputSampleRate();
}

bool QSynth::isActive() const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return false;
	}
	bool result = synth->isActive();
	synthMutex->unlock();
	return result;
}

bool QSynth::reset() {
	if (!isOpen()) return true;

	setState(SynthState_CLOSING);

	midiMutex.lock();
	synthMutex->lock();
	synth->close();
	// Do not delete synth here to keep the rendered frame counter value, audioStream is also alive during reset
	if (!synth->open(*controlROMImage, *pcmROMImage, actualAnalogOutputMode)) {
		// We're now in a partially-open state - better to properly close.
		synth->close(true);
		delete synth;
		synth = new Synth(&reportHandler);
		synthMutex->unlock();
		midiMutex.unlock();
		setState(SynthState_CLOSED);
		return false;
	}
	synthMutex->unlock();
	midiMutex.unlock();
	reportHandler.onDeviceReconfig();

	setState(SynthState_OPEN);
	return true;
}

void QSynth::setState(SynthState newState) {
	if (state == newState) return;
	state = newState;
	emit stateChanged(newState);
}

void QSynth::close() {
	if (!isOpen()) return;
	setState(SynthState_CLOSING);
	midiMutex.lock();
	synthMutex->lock();
	synth->close();
	// This effectively resets rendered frame counter, audioStream is also going down
	delete synth;
	synth = new Synth(&reportHandler);
	if (sampleRateConverter != NULL) {
		delete sampleRateConverter;
		sampleRateConverter = NULL;
	}
	synthMutex->unlock();
	midiMutex.unlock();
	setState(SynthState_CLOSED);
	freeROMImages();
}

void QSynth::getSynthProfile(SynthProfile &synthProfile) const {
	synthMutex->lock();
	synthProfile.romDir = romDir;
	synthProfile.controlROMFileName = controlROMFileName;
	synthProfile.pcmROMFileName = pcmROMFileName;
	synthProfile.controlROMImage = controlROMImage;
	synthProfile.pcmROMImage = pcmROMImage;
	synthProfile.emuDACInputMode = synth->getDACInputMode();
	synthProfile.midiDelayMode = synth->getMIDIDelayMode();
	synthProfile.analogOutputMode = analogOutputMode;
	synthProfile.reverbCompatibilityMode = reverbCompatibilityMode;
	synthProfile.outputGain = synth->getOutputGain();
	synthProfile.reverbOutputGain = synth->getReverbOutputGain();
	synthProfile.reverbEnabled = synth->isReverbEnabled();
	synthProfile.reverbOverridden = synth->isReverbOverridden();
	synthProfile.reverbMode = reverbMode;
	synthProfile.reverbTime = reverbTime;
	synthProfile.reverbLevel = reverbLevel;
	synthProfile.reversedStereoEnabled = synth->isReversedStereoEnabled();
	synthMutex->unlock();
}

void QSynth::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	synthProfileName = useSynthProfileName;
	romDir = synthProfile.romDir;
	controlROMFileName = synthProfile.controlROMFileName;
	pcmROMFileName = synthProfile.pcmROMFileName;
	if (controlROMImage == NULL || pcmROMImage == NULL) {
		freeROMImages();
		controlROMImage = synthProfile.controlROMImage;
		pcmROMImage = synthProfile.pcmROMImage;
	} else if (synthProfile.controlROMImage != NULL && synthProfile.pcmROMImage != NULL) {
		bool controlROMChanged = strcmp((char *)controlROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.controlROMImage->getROMInfo()->sha1Digest) != 0;
		bool pcmROMChanged = strcmp((char *)pcmROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.pcmROMImage->getROMInfo()->sha1Digest) != 0;
		if (controlROMChanged || pcmROMChanged) {
			freeROMImages();
			controlROMImage = synthProfile.controlROMImage;
			pcmROMImage = synthProfile.pcmROMImage;
			reset();
		}
	}
	setReverbCompatibilityMode(synthProfile.reverbCompatibilityMode);
	setMIDIDelayMode(synthProfile.midiDelayMode);
	setDACInputMode(synthProfile.emuDACInputMode);
	setAnalogOutputMode(synthProfile.analogOutputMode);
	setOutputGain(synthProfile.outputGain);
	setReverbOutputGain(synthProfile.reverbOutputGain);
	setReverbSettings(synthProfile.reverbMode, synthProfile.reverbTime, synthProfile.reverbLevel);
	setReverbEnabled(synthProfile.reverbEnabled);
	setReverbOverridden(synthProfile.reverbOverridden);
	setReversedStereoEnabled(synthProfile.reversedStereoEnabled);
}

void QSynth::freeROMImages() {
	// Ensure our ROM images get freed even if the synth is still in use
	const ROMImage *cri = controlROMImage;
	controlROMImage = NULL;
	const ROMImage *pri = pcmROMImage;
	pcmROMImage = NULL;
	Master::getInstance()->freeROMImages(cri, pri);
}

PartialState QSynth::getPartialState(int partialPhase) {
	static const PartialState partialPhaseToState[8] = {
		PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK,
		PartialState_SUSTAINED, PartialState_SUSTAINED, PartialState_RELEASED, PartialState_DEAD
	};
	return partialPhaseToState[partialPhase];
}

const QReportHandler *QSynth::getReportHandler() const {
	return &reportHandler;
}
