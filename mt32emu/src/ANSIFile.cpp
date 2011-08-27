/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#include "mt32emu.h"
#include "ANSIFile.h"

using namespace MT32Emu;
using namespace std;

ANSIFile::ANSIFile() {
	ifsp = new ifstream();
}

ANSIFile::~ANSIFile() {
	if (ifsp) {
		delete ifsp; // destructor closes the file itself
	}
	if (data) {
		delete[] data;
	}
}

size_t ANSIFile::getSize() {
	if (!ifsp) {
		return 0;
	}
	if (ifsp->bad()) {
		return 0;
	}
	ifsp->seekg(0, ios_base::end);
	return ifsp->tellg();
}

unsigned char* ANSIFile::getData() {
	if (!ifsp) {
		return NULL;
	}
	if (ifsp->bad()) {
		return NULL;
	}
	size_t fileSize = getSize();
	if (!fileSize) {
		return NULL;
	}
	data = new unsigned char[fileSize];
	if (data) {
		ifsp->seekg(0);
		ifsp->read((char *)data, fileSize);
		if ((size_t)ifsp->tellg() != fileSize) {
			return NULL;
		}
	}
	return data;
}

unsigned char* ANSIFile::getSHA1() {
	return NULL;
}

bool ANSIFile::open(const char *filename, OpenMode mode) {
	if (ifsp) {
		ifsp->open(filename, ios_base::in | ios_base::binary);
	}
	return (ifsp->good());
}

void ANSIFile::close() {
	ifsp->close();
}

size_t ANSIFile::read(void *in, size_t size) {
	size_t begin = ifsp->tellg();
	ifsp->read((char *)in, size);
	size_t end = ifsp->tellg();
	return end - begin;
}

bool ANSIFile::readBit8u(Bit8u *in) {
	char c = ifsp->get();
	if (ifsp->bad()) {
		return false;
	}
	*in = (Bit8u)c;
	return true;
}

bool ANSIFile::isEOF() {
	return ifsp->eof();
}
