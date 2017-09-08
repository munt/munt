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

#include "AudioDriver.h"
#include <QSettings>
#include "../Master.h"

AudioStream::AudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate) :
	synth(useSynth), sampleRate(useSampleRate), settings(useSettings), renderedFramesCount(0), lastEstimatedPlayedFramesCount(0), resetScheduled(true)
{
	audioLatencyFrames = settings.audioLatency * sampleRate / MasterClock::MILLIS_PER_SECOND;
	midiLatencyFrames = settings.midiLatency * sampleRate / MasterClock::MILLIS_PER_SECOND;
	timeInfoIx = 0;
	timeInfo[0].lastPlayedNanos = MasterClock::getClockNanos();
	timeInfo[0].lastPlayedFramesCount = renderedFramesCount;
	timeInfo[0].actualSampleRate = sampleRate;
	timeInfo[1] = timeInfo[0];
}

// Intended to be called from MIDI receiving thread
quint64 AudioStream::estimateMIDITimestamp(const MasterClockNanos refNanos) {
	MasterClockNanos midiNanos = (refNanos == 0) ? MasterClock::getClockNanos() : refNanos;
	uint i = timeInfoIx;
	if (midiNanos < timeInfo[i].lastPlayedNanos) {
		// Cross-boundary case, use previous time info for late events
		i = 1 - i;
	}
	quint64 refFrameOffset = quint64(((midiNanos - timeInfo[i].lastPlayedNanos) * timeInfo[i].actualSampleRate) / MasterClock::NANOS_PER_SECOND);
	quint64 timestamp = timeInfo[i].lastPlayedFramesCount + refFrameOffset + midiLatencyFrames;
	qint64 delay = qint64(timestamp - renderedFramesCount);
	if (delay < 0) {
		// Negative delay means our timing is broken. We want to absort all the jitter while keeping the latency at the minimum.
		if (isAutoLatencyMode()) {
			midiLatencyFrames -= delay;
		}
		qDebug() << "L" << renderedFramesCount << timestamp << delay << midiLatencyFrames;
	}
	return timestamp;
}

void AudioStream::updateTimeInfo(const MasterClockNanos measuredNanos, const quint32 framesInAudioBuffer) {
#if 0
	qDebug() << "R" << renderedFramesCount - timeInfo[timeInfoIx].lastPlayedFramesCount
					<< (measuredNanos - timeInfo[timeInfoIx].lastPlayedNanos) * 1e-6;
#endif

	if (((measuredNanos - timeInfo[timeInfoIx].lastPlayedNanos) * sampleRate) < (midiLatencyFrames * MasterClock::NANOS_PER_SECOND)) {
		// If callbacks are coming too quickly, we cannot benefit from that, it just makes our timing estimation worse.
		// This is because some audio systems may pull more data than the our specified audio latency in no time.
		// Moreover, we should be able to adjust lastPlayedFramesCount increasing speed as it counts in samples.
		// So, it seems reasonable to only update time info at intervals no less than our total MIDI latency,
		// which is meant to absort all the jitter.
		return;
	}
	uint nextTimeInfoIx = 1 - timeInfoIx;

	// Number of played frames (assuming no x-runs happend)
	quint64 estimatedNewPlayedFramesCount = settings.advancedTiming ? quint64(renderedFramesCount - framesInAudioBuffer) : renderedFramesCount;
	double secondsElapsed = double(measuredNanos - timeInfo[timeInfoIx].lastPlayedNanos) / MasterClock::NANOS_PER_SECOND;

	// Ensure lastPlayedFramesCount is monotonically increasing and has no jumps
	quint64 newPlayedFramesCount = timeInfo[timeInfoIx].lastPlayedFramesCount + quint64(timeInfo[timeInfoIx].actualSampleRate * secondsElapsed + 0.5);

	qint64 absError = qint64(estimatedNewPlayedFramesCount - newPlayedFramesCount);

	// If the estimation goes too far - do reset
	if (resetScheduled || qAbs(absError) > qint64(midiLatencyFrames)) {
		if (resetScheduled) {
			resetScheduled = false;
		} else {
			qDebug() << "AudioStream: Estimated play position is way off:" << absError << "-> resetting...";
		}
		lastEstimatedPlayedFramesCount = estimatedNewPlayedFramesCount;
		timeInfo[nextTimeInfoIx].lastPlayedNanos = measuredNanos;
		timeInfo[nextTimeInfoIx].lastPlayedFramesCount = estimatedNewPlayedFramesCount;
		timeInfo[nextTimeInfoIx].actualSampleRate = sampleRate;
		timeInfoIx = nextTimeInfoIx;
		return;
	}

	const double estimatedNewActualSampleRate = (estimatedNewPlayedFramesCount - lastEstimatedPlayedFramesCount + absError) / secondsElapsed;
	const double prevActualSampleRate = timeInfo[timeInfoIx].actualSampleRate;
	const double filteredNewActualSampleRate = prevActualSampleRate + (estimatedNewActualSampleRate - prevActualSampleRate) * 0.1;

	// Now fixup sample rate estimation. It shouldn't go too far from expected.
	// Assume the actual sample rate differs from nominal one within 1% range.
	// Actual hardware sample rates tend to be even more accurate as noted,
	// for example, in the paper http://www.portaudio.com/docs/portaudio_sync_acmc2003.pdf.
	// Although, software resampling can introduce more significant inaccuracies,
	// e.g. WinMME on my WinXP system works at about 32100Hz instead, while WASAPI, OSS, PulseAudio and ALSA perform much better.
	// Setting 0.5% as the maximum permitted relative error provides for superior rendering accuracy, and sample rate deviations should now be inaudible.
	// In case there are nasty environments with greater deviations in sample rate, we should make this configurable.
	double newActualSampleRate = qBound(0.995 * sampleRate, filteredNewActualSampleRate, 1.005 * sampleRate) ;

#if 0
	qDebug() << "S" << newActualSampleRate << absError;
#endif

	lastEstimatedPlayedFramesCount = estimatedNewPlayedFramesCount;
	timeInfo[nextTimeInfoIx].lastPlayedNanos = measuredNanos;
	timeInfo[nextTimeInfoIx].lastPlayedFramesCount = newPlayedFramesCount;
	timeInfo[nextTimeInfoIx].actualSampleRate = newActualSampleRate;
	timeInfoIx = nextTimeInfoIx;
}

bool AudioStream::isAutoLatencyMode() const {
	return settings.midiLatency == 0;
}

AudioDevice::AudioDevice(AudioDriver &useDriver, QString useName) : driver(useDriver), name(useName) {}

AudioDriver::AudioDriver(QString useID, QString useName) : id(useID), name(useName) {}

void AudioDriver::loadAudioSettings() {
	QSettings *qSettings = Master::getInstance()->getSettings();
	QString prefix = "Audio/" + id;
	settings.sampleRate = qSettings->value(prefix + "/SampleRate", 0).toUInt();
	settings.srcQuality = MT32Emu::SamplerateConversionQuality(qSettings->value(prefix + "/SRCQuality", MT32Emu::SamplerateConversionQuality_GOOD).toUInt());
	settings.chunkLen = qSettings->value(prefix + "/ChunkLen").toInt();
	settings.audioLatency = qSettings->value(prefix + "/AudioLatency").toInt();
	settings.midiLatency = qSettings->value(prefix + "/MidiLatency").toInt();
	settings.advancedTiming = qSettings->value(prefix + "/AdvancedTiming", true).toBool();
	validateAudioSettings(settings);
}

const AudioDriverSettings &AudioDriver::getAudioSettings() const {
	return settings;
}

void AudioDriver::setAudioSettings(AudioDriverSettings &useSettings) {
	validateAudioSettings(useSettings);
	settings = useSettings;

	QSettings *qSettings = Master::getInstance()->getSettings();
	QString prefix = "Audio/" + id;
	qSettings->setValue(prefix + "/SampleRate", settings.sampleRate);
	qSettings->setValue(prefix + "/SRCQuality", settings.srcQuality);
	qSettings->setValue(prefix + "/ChunkLen", settings.chunkLen);
	qSettings->setValue(prefix + "/AudioLatency", settings.audioLatency);
	qSettings->setValue(prefix + "/MidiLatency", settings.midiLatency);
	qSettings->setValue(prefix + "/AdvancedTiming", settings.advancedTiming);
}

void AudioDriver::migrateAudioSettingsFromVersion1() {
	QSettings *settings = Master::getInstance()->getSettings();
	foreach(QString group, settings->childGroups()) {
		if (group == "Master" || group == "Profiles") continue;
		QString oldPrefix = group + "/";
		QString newPrefix = "Audio/" + group + "/";
		settings->beginGroup(group);
		const QStringList keys = settings->childKeys();
		settings->endGroup();
		foreach(QString key, keys) {
			if (key == "MidiLatency") {
				bool advancedTiming = group == "waveout" || settings->value(oldPrefix + "AdvancedTiming", true).toBool();
				if (advancedTiming) {
					int midiLatency = settings->value(oldPrefix + "MidiLatency").toInt();
					if (midiLatency != 0) {
						int audioLatency = settings->value(oldPrefix + "AudioLatency").toInt();
						settings->setValue(newPrefix + key, midiLatency + audioLatency);
						continue;
					}
				}
			}
			settings->setValue(newPrefix + key, settings->value(oldPrefix + key));
		}
		settings->remove(group);
	}
}
