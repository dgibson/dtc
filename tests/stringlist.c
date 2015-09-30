/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase for string handling
 * Copyright (C) 2015 NVIDIA Corporation
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

static void check_expected_failure(const void *fdt, const char *path,
				   const char *property)
{
	int offset, err;

	offset = fdt_path_offset(fdt, "/");
	if (offset < 0)
		FAIL("Couldn't find path %s", path);

	err = fdt_stringlist_count(fdt, offset, "#address-cells");
	if (err != -FDT_ERR_BADVALUE)
		FAIL("unexpectedly succeeded in parsing #address-cells\n");
}

static void check_string_count(const void *fdt, const char *path,
			       const char *property, int count)
{
	int offset, err;

	offset = fdt_path_offset(fdt, path);
	if (offset < 0)
		FAIL("Couldn't find path %s", path);

	err = fdt_stringlist_count(fdt, offset, property);
	if (err < 0)
		FAIL("Couldn't count strings in property %s of node %s: %d\n",
		     property, path, err);

	if (err != count)
		FAIL("String count for property %s of node %s is %d instead of %d\n",
		     path, property, err, count);
}

int main(int argc, char *argv[])
{
	void *fdt;

	if (argc != 2)
		CONFIG("Usage: %s <dtb file>\n", argv[0]);

	test_init(argc, argv);
	fdt = load_blob(argv[1]);

	check_expected_failure(fdt, "/", "#address-cells");
	check_expected_failure(fdt, "/", "#size-cells");

	check_string_count(fdt, "/", "compatible", 1);
	check_string_count(fdt, "/device", "compatible", 2);
	check_string_count(fdt, "/device", "big-endian", 0);

	PASS();
}
