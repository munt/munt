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

#include "MidiDriver.h"

#include <QtGlobal>

MidiDriver::MidiDriver(Master *useMaster): master(useMaster), name("Unknown") {

}

MidiDriver::~MidiDriver() {
}

QString MidiDriver::getName() const {
	 return name;
}

void MidiDriver::setName(const QString &newName) {
	name = newName;
	emit nameChanged(name);
}

Master *MidiDriver::getMaster() {
	return master;
}
