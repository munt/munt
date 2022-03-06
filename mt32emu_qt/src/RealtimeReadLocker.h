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

#ifndef REALTIME_READ_LOCKER_H
#define REALTIME_READ_LOCKER_H

#include "QReadWriteLock"

// Similar to QReadLocker but intended to be used from a realtime thread that should never block.
class RealtimeReadLocker {
private:
	QReadWriteLock &lock;
	bool locked;

public:
	explicit RealtimeReadLocker(QReadWriteLock &useLock) : lock(useLock) {
		locked = lock.tryLockForRead();
	}

	~RealtimeReadLocker() {
		if (locked) lock.unlock();
		locked = false;
	}

	bool isLocked() {
		return locked;
	}
};

#endif // REALTIME_READ_LOCKER_H
