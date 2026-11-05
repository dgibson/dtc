// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase for fdt_get_path()
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

#define POISON	('\xff')

static void check_path_buf(void *fdt, const char *path, int pathlen, int buflen)
{
	int offset;
	char buf[buflen+1];
	int len;

	offset = fdt_path_offset(fdt, path);
	if (offset < 0)
		FAIL("Couldn't find path \"%s\": %s", path, fdt_strerror(offset));

	memset(buf, POISON, sizeof(buf)); /* poison the buffer */

	len = fdt_get_path(fdt, offset, buf, buflen);
	verbose_printf("get_path() %s -> %d -> %s\n", path, offset,
		       len >= 0 ? buf : "<error>");

	if (buflen <= pathlen) {
		if (len != -FDT_ERR_NOSPACE)
			FAIL("fdt_get_path([%d bytes]) returns %d with "
			     "insufficient buffer space", buflen, len);
	} else {
		if (len < 0)
			FAIL("fdt_get_path([%d bytes]): %s", buflen,
			     fdt_strerror(len));
		if (len != 0)
			FAIL("fdt_get_path([%d bytes]) returns %d "
			     "instead of 0", buflen, len);
		if (strcmp(buf, path) != 0)
			FAIL("fdt_get_path([%d bytes]) returns \"%s\" "
			     "instead of \"%s\"", buflen, buf, path);
	}

	if (buf[buflen] != POISON)
		FAIL("fdt_get_path([%d bytes]) overran buffer", buflen);
}

static void check_path(void *fdt, const char *path)
{
	int pathlen = strlen(path);

	check_path_buf(fdt, path, pathlen, 1024);
	check_path_buf(fdt, path, pathlen, pathlen+1);
	check_path_buf(fdt, path, pathlen, pathlen);
	check_path_buf(fdt, path, pathlen, 0);
	check_path_buf(fdt, path, pathlen, 2);
}

/*
 * An offset past the root node's FDT_END_NODE tag is not a node offset,
 * but fdt_next_node() still returns it rather than an error (see commit
 * ce4d9c0f "libfdt: Rework fdt_next_node()"), so fdt_get_path() has to
 * recognise it from the depth going negative.  Before it did, the path
 * trimming scanned back past the start of the buffer for such an offset.
 */
static void check_path_bad_offset(void *fdt)
{
	int offset, depth = 0, badoffset = 0;
	int buflen = 64;
	char *buf = xmalloc(buflen);
	int len, i;

	/* The offset fdt_next_node() reports once it has left the tree. */
	for (offset = 0; offset >= 0; offset = fdt_next_node(fdt, offset, &depth))
		badoffset = offset;

	for (i = 0; i < 8; i++) {
		memset(buf, POISON, buflen);

		len = fdt_get_path(fdt, badoffset + i, buf, buflen);
		if (len != -FDT_ERR_BADOFFSET)
			FAIL("fdt_get_path(%d) returns %d instead of "
			     "-FDT_ERR_BADOFFSET", badoffset + i, len);
	}

	free(buf);
}

int main(int argc, char *argv[])
{
	void *fdt;

	test_init(argc, argv);
	fdt = load_blob_arg(argc, argv);

	check_path(fdt, "/");
	check_path(fdt, "/subnode@1");
	check_path(fdt, "/subnode@2");
	check_path(fdt, "/subnode@1/subsubnode");
	check_path(fdt, "/subnode@2/subsubnode@0");

	check_path_bad_offset(fdt);

	PASS();
}
