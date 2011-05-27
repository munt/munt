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

#ifndef MIDI_SESSION_H
#define MIDI_SESSION_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;

class MidiSession : public QObject {
	Q_OBJECT
	friend class Master;
private:
	MidiDriver *midiDriver;
	QString name;
	SynthRoute *synthRoute;

	MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute);

public:
	QString getName();
	SynthRoute *getSynthRoute();
};

#endif
