// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase for misbehaviour on an unterminated memrsv map
 * Copyright Red Hat
 *
 * Based on a proof of concept report from:
 *   Moshe Strauss <moshestrauss10@gmail.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

#define SPACE		65536

int main(int argc, char *argv[])
{
	void *fdt = &unterminated_memrsv;
	void *buf;
	int err;
	uint64_t addr, size;

	test_init(argc, argv);

	err = fdt_check_header(fdt);
	if (err != 0)
		FAIL("Bad header: %s", fdt_strerror(err));

	err = fdt_num_mem_rsv(fdt);
	if (err != -FDT_ERR_TRUNCATED)
		FAIL("fdt_num_mem_rsv() returned %d instead of -FDT_ERR_TRUNCATED",
		     err);

	err = fdt_get_mem_rsv(fdt, 0, &addr, &size);
	if (err != 0)
		FAIL("fdt_get_mem_rsv() failed on first entry: %s",
		     fdt_strerror(err));
	if ((addr != TEST_ADDR_1) || (size != TEST_SIZE_1))
		FAIL("Entry doesn't match: (0x%llx, 0x%llx) != (0x%llx, 0x%llx)",
		     (unsigned long long)addr, (unsigned long long)size,
		     TEST_ADDR_1, TEST_SIZE_1);

	err = fdt_add_mem_rsv(fdt, TEST_ADDR_2, TEST_SIZE_2);
	if (err != -FDT_ERR_TRUNCATED)
		FAIL("fdt_add_mem_rsv() returned %d instead of -FDT_ERR_TRUNCATED",
		     err);

	err = fdt_del_mem_rsv(fdt, 0);
	if (err != -FDT_ERR_TRUNCATED)
		FAIL("fdt_del_mem_rsv() returned %d instead of -FDT_ERR_TRUNCATED",
		     err);
		
	buf = xmalloc(SPACE);
	err = fdt_open_into(fdt, buf, SPACE);
	if (err != -FDT_ERR_TRUNCATED)
		FAIL("fdt_open_into() returned %d instead of -FDT_ERR_TRUNCATED",
		     err);

	PASS();
}
