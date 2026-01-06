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

#ifndef MT32EMU_C_TEST_HARNESS_H
#define MT32EMU_C_TEST_HARNESS_H

#include "../c_interface/c_types.h"

#define MT32EMU_FAIL_TEST(reason) mt32emu_fail_test(__FILE__, __LINE__, (reason))
#define MT32EMU_ASSERT_INT_EQ(value1, value2) mt32emu_assert_int_eq(__FILE__, __LINE__, \
	#value1 "=", #value2 "=", (value1), (value2))
#define MT32EMU_ASSERT_STR_EQ(value1, value2) mt32emu_assert_str_eq(__FILE__, __LINE__, \
	#value1 "=", #value2 "=", (value1), (value2))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mt32emu_rom_image {
	const mt32emu_bit8u *data;
	size_t data_size;
	const mt32emu_sha1_digest *sha1_digest;
} mt32emu_rom_image;

void mt32emu_fail_test(const char *file, int line, const char *reason);
void mt32emu_assert_int_eq(const char *file, int line, const char *desc1, const char *desc2, int value1, int value2);
void mt32emu_assert_str_eq(const char *file, int line, const char *desc1, const char *desc2, const char *value1, const char *value2);

void mt32emu_create_rom_set(const char *machine_id, mt32emu_rom_image *control_rom_image, mt32emu_rom_image *pcm_rom_image);
void mt32emu_free_rom_set(const mt32emu_rom_image *control_rom_image, const mt32emu_rom_image *pcm_rom_image);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* #ifndef MT32EMU_C_TEST_HARNESS_H */
