/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

void AudioDriver::loadAudioSettings() {
	QSettings *settings = Master::getInstance()->getSettings();
	chunkLen = settings->value("waveout/ChunkLen").toInt();
	audioLatency = settings->value(id + "/AudioLatency").toInt();
	midiLatency = settings->value(id + "/MidiLatency").toInt();
	validateAudioSettings();
}

void AudioDriver::getAudioSettings(unsigned int *pChunkLen, unsigned int *pAudioLatency, unsigned int *pMidiLatency) const {
	*pChunkLen = chunkLen;
	*pAudioLatency = audioLatency;
	*pMidiLatency = midiLatency;
}

void AudioDriver::setAudioSettings(unsigned int *pChunkLen, unsigned int *pAudioLatency, unsigned int *pMidiLatency) {
	chunkLen = *pChunkLen;
	audioLatency = *pAudioLatency;
	midiLatency = *pMidiLatency;

	validateAudioSettings();

	*pChunkLen = chunkLen;
	*pAudioLatency = audioLatency;
	*pMidiLatency = midiLatency;

	QSettings *settings = Master::getInstance()->getSettings();
	settings->setValue(id + "/ChunkLen", chunkLen);
	settings->setValue(id + "/AudioLatency", audioLatency);
	settings->setValue(id + "/MidiLatency", midiLatency);
}
