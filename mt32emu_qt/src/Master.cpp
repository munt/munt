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

#include "Master.h"
#include "MidiSession.h"

Master *Master::INSTANCE = NULL;

void Master::init() {
	if(INSTANCE == NULL) {
		INSTANCE = new Master();
	}
}

void Master::deinit() {
	delete INSTANCE;
	INSTANCE = NULL;
}

MidiSession *Master::createMidiSession(MidiDriver *midiDriver, QString name) {
	// For now, we always create a new SynthRoute for each session
	// In future this will be configurable
	SynthRoute *synthRoute = new SynthRoute();
	MidiSession *midiSession = new MidiSession(midiDriver, name, synthRoute);
	synthRoute->addMidiSession(midiSession);
	synthRoutes.append(synthRoute);
	synthRoute->open();
	emit synthRouteAdded(synthRoute);
	return midiSession;
}

void Master::deleteMidiSession(MidiSession *midiSession) {
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	synthRoute->removeMidiSession(midiSession);
	if(!synthRoute->isPinned()) {
		synthRoutes.removeOne(synthRoute);
		emit synthRouteRemoved(synthRoute);
		synthRoute->close();
		delete synthRoute;
	}
	delete midiSession;
}

Master *Master::getInstance() {
	return INSTANCE;
}

Master::Master() {
}
