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

#include <stdio.h>

#include "mt32emu.h"

namespace MT32Emu {

	bool ANSIFile::open(const char *filename, OpenMode mode) {
		const char *fmode;
		if (mode == OpenMode_read) {
			fmode = "rb";
		} else {
			fmode = "wb";
		}
		fp = fopen(filename, fmode);
		return (fp != NULL);
	}

	void ANSIFile::close() {
		fclose(fp);
	}

	size_t ANSIFile::read(void *in, size_t size) {
		return fread(in, 1, size, fp);
	}

	bool ANSIFile::readLine(char *in, size_t size) {
		return fgets(in, (int)size, fp) != NULL;
	}

	bool ANSIFile::readBit8u(Bit8u *in) {
		int c = fgetc(fp);
		if (c == EOF)
			return false;
		*in = (Bit8u)c;
		return true;
	}

	bool File::readBit16u(Bit16u *in) {
		Bit8u b[2];
		if (read(&b[0], 2) != 2)
			return false;
		*in = ((b[0] << 8) | b[1]);
		return true;
	}

	bool File::readBit32u(Bit32u *in) {
		Bit8u b[4];
		if (read(&b[0], 4) != 4)
			return false;
		*in = ((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
		return true;
	}

	size_t ANSIFile::write(const void *out, size_t size) {
		return fwrite(out, 1, size, fp);
	}

	bool ANSIFile::writeBit8u(Bit8u out) {
		return fputc(out, fp) != EOF;
	}

	bool File::writeBit16u(Bit16u out) {
		if (!writeBit8u((Bit8u)((out & 0xFF00) >> 8))) {
			return false;
		}
		if (!writeBit8u((Bit8u)(out & 0x00FF))) {
			return false;
		}
		return true;
	}

	bool File::writeBit32u(Bit32u out) {
		if (!writeBit8u((Bit8u)((out & 0xFF000000) >> 24))) {
			return false;
		}
		if (!writeBit8u((Bit8u)((out & 0x00FF0000) >> 16))) {
			return false;
		}
		if (!writeBit8u((Bit8u)((out & 0x0000FF00) >> 8))) {
			return false;
		}
		if (!writeBit8u((Bit8u)(out & 0x000000FF))) {
			return false;
		}
		return true;
	}

	bool ANSIFile::isEOF() {
		return feof(fp) != 0;
	}
}
