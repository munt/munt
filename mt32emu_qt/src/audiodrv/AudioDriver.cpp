/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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

#include "AudioDriver.h"
#include "../Master.h"
#include <QSettings>

AudioDevice::AudioDevice(const AudioDriver * const useDriver, QString useID, QString useName) : driver(useDriver), id(useID), name(useName) {
}

AudioDriver::AudioDriver(QString useID, QString useName) : id(useID), name(useName) {
}

double AudioStream::estimateActualSampleRate(const double sampleRate, MasterClockNanos &firstSampleMasterClockNanos, MasterClockNanos &lastSampleMasterClockNanos, const MasterClockNanos audioLatency, const quint32 frameCount) {
	MasterClockNanos newFirstSampleMasterClockNanos = firstSampleMasterClockNanos;
	// Ensure rendering time function has no breaks while no x-runs happen
	if (qAbs(firstSampleMasterClockNanos - lastSampleMasterClockNanos) < audioLatency) {
		firstSampleMasterClockNanos = lastSampleMasterClockNanos;
	}
	// Estimate rendering time using nominal sample rate
	MasterClockNanos nominalNanosToRender = MasterClockNanos(MasterClock::NANOS_PER_SECOND * frameCount / sampleRate);
	// Ensure outputBufferDacTime estimation doesn't go too far from expected.
	// Assume the real sample rate differs from nominal one no more than by 1%.
	// Actual hardware sample rates tend to be even more accurate as noted,
	// for example, in the paper http://www.portaudio.com/docs/portaudio_sync_acmc2003.pdf.
	// Although, software resampling can introduce more significant inaccuracies,
	// e.g. WinMME on my WinXP system works at about 32100Hz instead, while WASAPI, OSS, PulseAudio and ALSA perform much better.
	// Setting 1% as the maximum relative error provides for superior rendering accuracy, and sample rate deviations should now be inaudible.
	// In case there are nasty environments with greater deviations in sample rate, we should make this configurable.
	MasterClockNanos nanosToRender = (newFirstSampleMasterClockNanos + nominalNanosToRender) - firstSampleMasterClockNanos;
	double relativeError = (double)nanosToRender / nominalNanosToRender;
	if (relativeError < 0.99) {
		nanosToRender = 0.99 * nominalNanosToRender;
	}
	if (relativeError > 1.01) {
		nanosToRender = 1.01 * nominalNanosToRender;
	}
	lastSampleMasterClockNanos = firstSampleMasterClockNanos + nanosToRender;
	// Compute actual sample rate so that the actual rendering time interval ends exactly in lastSampleMasterClockNanos point
	return MasterClock::NANOS_PER_SECOND * (double)frameCount / (double)nanosToRender;
}

void AudioDriver::loadAudioSettings() {
	QSettings *settings = Master::getInstance()->getSettings();
	chunkLen = settings->value(id + "/ChunkLen").toInt();
	audioLatency = settings->value(id + "/AudioLatency").toInt();
	midiLatency = settings->value(id + "/MidiLatency").toInt();
	advancedTiming = settings->value(id + "/AdvancedTiming", true).toBool();
	validateAudioSettings();
}

void AudioDriver::getAudioSettings(unsigned int *pChunkLen, unsigned int *pAudioLatency, unsigned int *pMidiLatency, bool *pAdvancedTiming) const {
	*pChunkLen = chunkLen;
	*pAudioLatency = audioLatency;
	*pMidiLatency = midiLatency;
	*pAdvancedTiming = advancedTiming;
}

void AudioDriver::setAudioSettings(unsigned int *pChunkLen, unsigned int *pAudioLatency, unsigned int *pMidiLatency, bool *pAdvancedTiming) {
	chunkLen = *pChunkLen;
	audioLatency = *pAudioLatency;
	midiLatency = *pMidiLatency;
	advancedTiming = *pAdvancedTiming;

	validateAudioSettings();

	*pChunkLen = chunkLen;
	*pAudioLatency = audioLatency;
	*pMidiLatency = midiLatency;
	*pAdvancedTiming = advancedTiming;

	QSettings *settings = Master::getInstance()->getSettings();
	settings->setValue(id + "/ChunkLen", chunkLen);
	settings->setValue(id + "/AudioLatency", audioLatency);
	settings->setValue(id + "/MidiLatency", midiLatency);
	settings->setValue(id + "/AdvancedTiming", advancedTiming);
}
