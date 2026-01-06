/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2024 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#include <cstring>

#include "../ROMInfo.h"

#include "FakeROMs.h"
#include "Testing.h"
#include "c_client_test.h"
#include "c_test_harness.h"

namespace MT32Emu {

namespace Test {

TEST_CASE("C code creates service interface and checks its version") {
	mt32emu_test_service_interface();
}

TEST_CASE("C code retrieves library version") {
	mt32emu_test_library_version();
}

TEST_CASE("C code creates context but can't open synth without ROMs") {
	mt32emu_test_open_synth_without_roms();
}

TEST_CASE("C code creates context and opens synth with ROMs") {
	mt32emu_test_open_synth_with_roms();
}

} // namespace Test

} // namespace MT32Emu

using namespace MT32Emu;
using namespace MT32Emu::Test;

extern "C" {

void mt32emu_fail_test(const char *file, int line, const char *reason) {
	ADD_FAIL_CHECK_AT(file, line, reason);
}

void mt32emu_assert_int_eq(const char *file, int line, const char *desc1, const char *desc2, int value1, int value2) {
	INFO(desc1 << value1);
	INFO(desc2 << value2);
	if (value1 == value2) {
		CHECK(true);
	} else {
		ADD_FAIL_CHECK_AT(file, line, "Integers are NOT equal!");
	}
}

void mt32emu_assert_str_eq(const char *file, int line, const char *desc1, const char *desc2, const char *value1, const char *value2) {
	INFO(desc1 << value1);
	INFO(desc2 << value2);
	if (strcmp(value1, value2) == 0) {
		CHECK(true);
	} else {
		ADD_FAIL_CHECK_AT(file, line, "Strings are NOT equal!");
	}
}

void mt32emu_create_rom_set(const char *machine_id, mt32emu_rom_image *control_rom_image, mt32emu_rom_image *pcm_rom_image) {
	ROMSet romSet;
	romSet.init(machine_id);
	const ROMImage *controlROMImage = romSet.getControlROMImage();
	const ROMImage *pcmROMImage = romSet.getPCMROMImage();

	CHECK(controlROMImage != NULL_PTR);
	CHECK(pcmROMImage != NULL_PTR);
	if (NULL == controlROMImage || NULL == pcmROMImage) {
		control_rom_image->data = NULL;
		pcm_rom_image->data = NULL;
		return;
	}

	control_rom_image->data_size = controlROMImage->getFile()->getSize();
	mt32emu_bit8u *controlROMData = new mt32emu_bit8u[control_rom_image->data_size];
	control_rom_image->data = controlROMData;
	memcpy(controlROMData, controlROMImage->getFile()->getData(), control_rom_image->data_size);
	control_rom_image->sha1_digest = &controlROMImage->getROMInfo()->sha1Digest;

	pcm_rom_image->data_size = pcmROMImage->getFile()->getSize();
	mt32emu_bit8u *pcmROMData = new mt32emu_bit8u[pcm_rom_image->data_size];
	pcm_rom_image->data = pcmROMData;
	memcpy(pcmROMData, pcmROMImage->getFile()->getData(), pcm_rom_image->data_size);
	pcm_rom_image->sha1_digest = &pcmROMImage->getROMInfo()->sha1Digest;
}

void mt32emu_free_rom_set(const mt32emu_rom_image *control_rom_image, const mt32emu_rom_image *pcm_rom_image) {
	delete[] control_rom_image->data;
	delete[] pcm_rom_image->data;
}

} // extern "C"
