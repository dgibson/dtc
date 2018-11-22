/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase for fdt_getprop_by_offset()
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

int main(int argc, char *argv[])
{
	bool found_prop_int = false;
	bool found_prop_str = false;
	int poffset;
	void *fdt;

	test_init(argc, argv);
	fdt = load_blob_arg(argc, argv);

	fdt_for_each_property_offset(poffset, fdt, 0) {
		if (check_get_prop_offset_cell(fdt, poffset, "prop-int",
					       TEST_VALUE_1))
			found_prop_int = true;
		if (check_get_prop_offset(fdt, poffset, "prop-str",
					  strlen(TEST_STRING_1) + 1,
					  TEST_STRING_1))
			found_prop_str = true;
	}
	if (!found_prop_int)
		FAIL("Property 'prop-int' not found");
	if (!found_prop_str)
		FAIL("Property 'prop-str' not found");

	PASS();
}
