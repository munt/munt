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

#ifndef QATOMIC_HELPER_H
#define QATOMIC_HELPER_H

#include "QAtomicInt"
#include "QAtomicPointer"

// The Qt API for atomic access looks a bit unstable yet.
// This helper wraps the atomic load or store operations that suddenly got
// renamed in Qt 5.14 and were completely unavailable in the old Qt4 API.
// Although, the implementation suggests that the available "non-atomic"
// operations in Qt4 involve aligned volatile access which should be atomic.
// To enforce memory ordering, we have to invoke a real atomic operation though.

namespace QAtomicHelper {

static inline quint32 loadRelaxed(const QAtomicInt &atomicInt) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	// Note, if this appears non-atomic on some unsupported platform, a workaround
	// might be something like fetchAndAddRelaxed(0).
	return atomicInt;
#elif QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return atomicInt.load();
#else
	return atomicInt.loadRelaxed();
#endif
}

static inline quint32 loadAcquire(const QAtomicInt &atomicInt) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QAtomicInt helper(atomicInt);
	return helper.fetchAndStoreAcquire(0);
#else
	return atomicInt.loadAcquire();
#endif
}

static inline void storeRelease(QAtomicInt &atomicInt, quint32 value) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	atomicInt.fetchAndStoreRelease(value);
#else
	atomicInt.storeRelease(value);
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
template <class T>
static inline T *loadRelaxed(QAtomicPointer<T> &atomicPointer) {
	return atomicPointer.fetchAndAddRelaxed(0);
#else
template <class T>
static inline T *loadRelaxed(const QAtomicPointer<T> &atomicPointer) {
#  if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return atomicPointer.load();
#  else
	return atomicPointer.loadRelaxed();
#  endif
#endif // QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
}

template <class T>
static inline void storeRelease(QAtomicPointer<T> &atomicPointer, T *value) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	atomicPointer.fetchAndStoreRelease(value);
#else
	atomicPointer.storeRelease(value);
#endif
}

} // namespace QAtomicHelper

#endif // QATOMIC_HELPER_H
