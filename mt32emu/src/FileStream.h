/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2015 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_FILE_STREAM_H
#define MT32EMU_FILE_STREAM_H

#include <fstream>
#include <iostream>
#include <cstdio>

#include "File.h"

namespace MT32Emu {

class FileStream : public AbstractFile {
public:
	FileStream();
	~FileStream();
	size_t getSize();
	const Bit8u *getData();
	bool open(const char *filename);
	void close();

private:
	std::ifstream ifsp;
	const Bit8u *data;
	size_t size;
};

}

#endif
