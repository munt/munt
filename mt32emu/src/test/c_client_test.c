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

#include <stddef.h>

#include "../mt32emu.h"

#include "c_client_test.h"
#include "c_test_harness.h"

void mt32emu_test_service_interface(void) {
	mt32emu_service_i service_i = mt32emu_get_service_i();
	const mt32emu_service_i_v0 *service = service_i.v0;
	mt32emu_service_version service_version = service->getVersionID(service_i);

	MT32EMU_ASSERT_INT_EQ(MT32EMU_SERVICE_VERSION_CURRENT, service_version);
}

void mt32emu_test_library_version(void) {
	MT32EMU_ASSERT_STR_EQ(MT32EMU_VERSION, mt32emu_get_library_version_string());
	MT32EMU_ASSERT_INT_EQ(MT32EMU_CURRENT_VERSION_INT, mt32emu_get_library_version_int());
}

void mt32emu_test_open_synth_without_roms(void) {
	mt32emu_report_handler_i rh = { NULL };
	mt32emu_context context = mt32emu_create_context(rh, NULL);
	mt32emu_return_code rc;

	if (NULL == context) {
		MT32EMU_FAIL_TEST("Failed to create context (got NULL)");
		return;
	}

	rc = mt32emu_open_synth(context);
	mt32emu_free_context(context);
	MT32EMU_ASSERT_INT_EQ(MT32EMU_RC_MISSING_ROMS, rc);
}

void mt32emu_test_open_synth_with_roms(void) {
	mt32emu_rom_image control_rom_image;
	mt32emu_rom_image pcm_rom_image;
	mt32emu_rom_info rom_info;
	mt32emu_return_code rc;
	mt32emu_report_handler_i rh = { NULL };
	mt32emu_context context = mt32emu_create_context(rh, NULL);

	if (NULL == context) {
		MT32EMU_FAIL_TEST("Failed to create context (got NULL)");
		return;
	}

	mt32emu_create_rom_set("mt32_1_07", &control_rom_image, &pcm_rom_image);
	if (NULL == control_rom_image.data || NULL == pcm_rom_image.data) {
		mt32emu_free_context(context);
		MT32EMU_FAIL_TEST("Failed to create ROM set for old-gen MT-32");
		return;
	}

	rc = mt32emu_add_rom_data(context, control_rom_image.data, control_rom_image.data_size, control_rom_image.sha1_digest);
	MT32EMU_ASSERT_INT_EQ(MT32EMU_RC_ADDED_CONTROL_ROM, rc);

	rc = mt32emu_add_rom_data(context, pcm_rom_image.data, pcm_rom_image.data_size, pcm_rom_image.sha1_digest);
	MT32EMU_ASSERT_INT_EQ(MT32EMU_RC_ADDED_PCM_ROM, rc);

	mt32emu_get_rom_info(context, &rom_info);
	MT32EMU_ASSERT_STR_EQ("ctrl_mt32_1_07", rom_info.control_rom_id);
	MT32EMU_ASSERT_STR_EQ("pcm_mt32", rom_info.pcm_rom_id);

	rc = mt32emu_open_synth(context);
	MT32EMU_ASSERT_INT_EQ(MT32EMU_RC_OK, rc);
	MT32EMU_ASSERT_INT_EQ(MT32EMU_BOOL_TRUE, mt32emu_is_open(context));

	mt32emu_free_context(context);
	mt32emu_free_rom_set(&control_rom_image, &pcm_rom_image);
}
