/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#ifndef MASTER_H
#define MASTER_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;
class MidiSession;

class Master : public QObject {
	Q_OBJECT
private:
	static Master *INSTANCE;

	QList<SynthRoute *> synthRoutes;
	QList<AudioDevice *> audioDevices;

	Master();

public:
	MidiSession *createMidiSession(MidiDriver *midiDriver, QString name);
	void deleteMidiSession(MidiSession *midiSession);
	void addAudioDevice(AudioDevice *audioDevice);
	void removeAudioDevice(AudioDevice *audioDevice);
	// May only be called from the application thread
	const QList<AudioDevice *> getAudioDevices() const;

	static Master *getInstance();

	// These two methods are only intended to be called at application startup/shutdown
	static void init();
	static void deinit();

private slots:
	void reallyCreateMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void reallyDeleteMidiSession(MidiSession *midiSession);
	void reallyAddAudioDevice(AudioDevice *audioDevice);
	void reallyRemoveAudioDevice(AudioDevice *audioDevice);

signals:
	void synthRouteAdded(SynthRoute *route);
	void synthRouteRemoved(SynthRoute *route);
	void audioDeviceAdded(AudioDevice *audioDevice);
	void audioDeviceRemoved(AudioDevice *audioDevice);
};

#endif
