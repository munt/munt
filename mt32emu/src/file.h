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

#ifndef MT32EMU_FILE_H
#define MT32EMU_FILE_H

#include <stdio.h>

namespace MT32Emu {

class File {
public:
	enum OpenMode {
		OpenMode_read  = 0,
		OpenMode_write = 1
	};
	virtual ~File() {}
	virtual void close() = 0;
	virtual size_t read(void *in, size_t size) = 0;
	virtual bool readLine(char *in, size_t size) = 0;
	virtual bool readBit8u(Bit8u *in) = 0;
	virtual bool readBit16u(Bit16u *in);
	virtual bool readBit32u(Bit32u *in);
	virtual size_t write(const void *out, size_t size) = 0;
	virtual bool writeBit8u(Bit8u out) = 0;
	// Note: May write a single byte to the file before failing
	virtual bool writeBit16u(Bit16u out);
	// Note: May write some (<4) bytes to the file before failing
	virtual bool writeBit32u(Bit32u out);
	virtual bool isEOF() = 0;
};

class ANSIFile: public File {
private:
	FILE *fp;
public:
	bool open(const char *filename, OpenMode mode);
	void close();
	size_t read(void *in, size_t size);
	bool readLine(char *in, size_t size);
	bool readBit8u(Bit8u *in);
	size_t write(const void *out, size_t size);
	bool writeBit8u(Bit8u out);
	bool isEOF();
};

}

#endif
