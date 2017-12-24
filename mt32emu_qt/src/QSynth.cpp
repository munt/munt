/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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
#include "AudioFileWriter.h"
#include "Master.h"
#include "MasterClock.h"

using namespace MT32Emu;

const int MIN_PARTIAL_COUNT = 8;
const int MAX_PARTIAL_COUNT = 256;

static const ROMImage *makeROMImage(const QDir &romDir, QString romFileName) {
	FileStream *file = new FileStream;
	if (file->open(Master::getROMPathName(romDir, romFileName).toLocal8Bit())) {
		return ROMImage::makeROMImage(file);
	}
	return NULL;
}

QReportHandler::QReportHandler(QObject *parent) : QObject(parent) {
	connect(this, SIGNAL(balloonMessageAppeared(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

void QReportHandler::printDebug(const char *fmt, va_list list) {
	qDebug() << "MT32:" << QString().vsprintf(fmt, list);
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

void QReportHandler::onPolyStateChanged(Bit8u partNum) {
	emit polyStateChanged(partNum);
}

void QReportHandler::onProgramChanged(Bit8u partNum, const char soundGroupName[], const char patchName[]) {
	emit programChanged(partNum, QString().fromLocal8Bit(soundGroupName), QString().fromLocal8Bit(patchName));
}

QSynth::QSynth(QObject *parent) :
	QObject(parent), state(SynthState_CLOSED), midiMutex(QMutex::Recursive),
	controlROMImage(NULL), pcmROMImage(NULL), reportHandler(this), sampleRateConverter(NULL), audioRecorder(NULL)
{
	synthMutex = new QMutex(QMutex::Recursive);
	synth = new Synth(&reportHandler);
}

QSynth::~QSynth() {
	freeROMImages();
	delete audioRecorder;
	delete sampleRateConverter;
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
	return Bit32u(sampleRateConverter->convertOutputToSynthTimestamp(timestamp));
}

void QSynth::render(Bit16s *buffer, uint length) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();

		// Synth is closed, simply erase buffer content
		memset(buffer, 0, length << 2);
		emit audioBlockRendered();
		return;
	}
	sampleRateConverter->getOutputSamples(buffer, length);
	if (isRecordingAudio()) {
		if (!audioRecorder->write(buffer, length)) stopRecordingAudio();
	}
	synthMutex->unlock();
	emit audioBlockRendered();
}

bool QSynth::open(uint &targetSampleRate, SamplerateConversionQuality srcQuality, const QString useSynthProfileName) {
	if (isOpen()) return true;

	if (!useSynthProfileName.isEmpty()) synthProfileName = useSynthProfileName;
	SynthProfile synthProfile;

	forever {
		Master::getInstance()->loadSynthProfile(synthProfile, synthProfileName);
		if (controlROMImage == NULL || pcmROMImage == NULL) Master::getInstance()->findROMImages(synthProfile, controlROMImage, pcmROMImage);
		if (controlROMImage == NULL) controlROMImage = makeROMImage(synthProfile.romDir, synthProfile.controlROMFileName);
		if (controlROMImage != NULL && pcmROMImage == NULL) pcmROMImage = makeROMImage(synthProfile.romDir, synthProfile.pcmROMFileName);
		if (controlROMImage != NULL && pcmROMImage != NULL) break;
		qDebug() << "Missing ROM files. Can't open synth :(";
		freeROMImages();
		if (!Master::getInstance()->handleROMSLoadFailed(synthProfileName)) return false;
	}

	MT32Emu::AnalogOutputMode actualAnalogOutputMode = synthProfile.analogOutputMode;
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

void QSynth::setNiceAmpRampEnabled(bool enabled) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setNiceAmpRampEnabled(enabled);
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

void QSynth::setInitialMIDIChannelsAssignment(bool engageChannel1) {
	engageChannel1OnOpen = engageChannel1;
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
		mt32CompatibleReverb = synth->isDefaultReverbMT32Compatible();
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

void QSynth::setRendererType(MT32Emu::RendererType useRendererType) {
	synth->selectRendererType(useRendererType);
}

void QSynth::setPartialCount(int newPartialCount) {
	partialCount = qBound(MIN_PARTIAL_COUNT, newPartialCount, MAX_PARTIAL_COUNT);
}

const QString QSynth::getPatchName(int partNum) const {
	synthMutex->lock();
	QString name = isOpen() ? QString().fromLocal8Bit(synth->getPatchName(partNum)) : QString("Channel %1").arg(partNum + 1);
	synthMutex->unlock();
	return name;
}

void QSynth::getPartStates(bool *partStates) const {
	synthMutex->lock();
	if (isOpen()) {
		synth->getPartStates(partStates);
	}
	synthMutex->unlock();
}

void QSynth::getPartialStates(MT32Emu::PartialState *partialStates) const {
	synthMutex->lock();
	if (isOpen()) {
		synth->getPartialStates(partialStates);
	}
	synthMutex->unlock();
}

unsigned int QSynth::getPlayingNotes(unsigned int partNumber, Bit8u *keys, Bit8u *velocities) const {
	unsigned int playingNotes = 0;
	synthMutex->lock();
	playingNotes = synth->getPlayingNotes(partNumber, keys, velocities);
	synthMutex->unlock();
	return playingNotes;
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
	static Bit8u sysex[] = { 0x7f, 0, 0 };

	midiMutex.lock();
	synthMutex->lock();
	if (isOpen()) {
		setState(SynthState_CLOSING);
		synth->writeSysex(16, sysex, 3);
		setState(SynthState_OPEN);
	}
	synthMutex->unlock();
	midiMutex.unlock();
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
	delete sampleRateConverter;
	sampleRateConverter = NULL;
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
	synthProfile.emuDACInputMode = synth->getDACInputMode();
	synthProfile.midiDelayMode = synth->getMIDIDelayMode();
	synthProfile.analogOutputMode = analogOutputMode;
	synthProfile.rendererType = synth->getSelectedRendererType();
	synthProfile.partialCount = partialCount;
	synthProfile.reverbCompatibilityMode = reverbCompatibilityMode;
	synthProfile.outputGain = synth->getOutputGain();
	synthProfile.reverbOutputGain = synth->getReverbOutputGain();
	synthProfile.reverbOverridden = synth->isReverbOverridden();
	synthProfile.reverbEnabled = synth->isReverbEnabled() || !synthProfile.reverbOverridden;
	synthProfile.reverbMode = reverbMode;
	synthProfile.reverbTime = reverbTime;
	synthProfile.reverbLevel = reverbLevel;
	synthProfile.reversedStereoEnabled = synth->isReversedStereoEnabled();
	synthProfile.niceAmpRamp = synth->isNiceAmpRampEnabled();
	synthProfile.engageChannel1OnOpen = engageChannel1OnOpen;
	synthMutex->unlock();
}

void QSynth::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	synthProfileName = useSynthProfileName;

	// Settings below do not take effect before re-open.
	romDir = synthProfile.romDir;
	controlROMFileName = synthProfile.controlROMFileName;
	pcmROMFileName = synthProfile.pcmROMFileName;
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
	setInitialMIDIChannelsAssignment(synthProfile.engageChannel1OnOpen);
}

void QSynth::getROMImages(const MT32Emu::ROMImage *&cri, const MT32Emu::ROMImage *&pri) const {
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
	synthMutex->lock();
	stopRecordingAudio();
	audioRecorder = new AudioFileWriter(sampleRateConverter->convertSynthToOutputTimestamp(SAMPLE_RATE), fileName);
	audioRecorder->open();
	synthMutex->unlock();
}

void QSynth::stopRecordingAudio() {
	synthMutex->lock();
	if (!isRecordingAudio()) {
		synthMutex->unlock();
		return;
	}
	audioRecorder->close();
	delete audioRecorder;
	audioRecorder = NULL;
	synthMutex->unlock();
}

bool QSynth::isRecordingAudio() const {
	return audioRecorder != NULL;
}
