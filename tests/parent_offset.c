// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * libfdt - Flat Device Tree manipulation
 *	Testcase for fdt_parent_offset()
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

static int path_parent_len(const char *path)
{
	const char *p = strrchr(path, '/');

	if (!p)
		TEST_BUG();
	if (p == path)
		return 1;
	else
		return p - path;
}

static void check_path(struct fdt_header *fdt, const char *path)
{
	char *parentpath;
	int nodeoffset, parentoffset, parentpathoffset, pathparentlen;

	pathparentlen = path_parent_len(path);
	parentpath = alloca(pathparentlen + 1);
	strncpy(parentpath, path, pathparentlen);
	parentpath[pathparentlen] = '\0';

	verbose_printf("Path: \"%s\"\tParent: \"%s\"\n", path, parentpath);

	nodeoffset = fdt_path_offset(fdt, path);
	if (nodeoffset < 0)
		FAIL("fdt_path_offset(%s): %s", path, fdt_strerror(nodeoffset));

	parentpathoffset = fdt_path_offset(fdt, parentpath);
	if (parentpathoffset < 0)
		FAIL("fdt_path_offset(%s): %s", parentpath,
		     fdt_strerror(parentpathoffset));

	parentoffset = fdt_parent_offset(fdt, nodeoffset);
	if (parentoffset < 0)
		FAIL("fdt_parent_offset(): %s", fdt_strerror(parentoffset));

	if (parentoffset != parentpathoffset)
		FAIL("fdt_parent_offset() returns %d instead of %d",
		     parentoffset, parentpathoffset);
}

/*
 * Check that an offset which does not name a node never yields a parent.
 * Sweep the whole blob, plus a little either side of it
 */
static void check_bad_offsets(struct fdt_header *fdt)
{
	int offset, size = fdt_totalsize(fdt);

	for (offset = -8; offset < size + 64; offset++) {
		int parentoffset;

		if (fdt_get_name(fdt, offset, NULL))
			continue;	/* a real node, checked elsewhere */

		parentoffset = fdt_parent_offset(fdt, offset);
		if (parentoffset >= 0)
			FAIL("fdt_parent_offset(%d) returns %d for an offset "
			     "which is not a node", offset, parentoffset);
	}
}

#define DEEP_SPACE	65536
#define DEEP_LEVELS	40

#define CHECK(code) \
	do { \
		int err_ = (code); \
		if (err_) \
			FAIL(#code ": %s", fdt_strerror(err_)); \
	} while (0)

/* Add a node holding a single subnode, as a side branch of the deep tree */
static void add_branch(void *fdt, const char *name, const char *child)
{
	CHECK(fdt_begin_node(fdt, name));
	CHECK(fdt_begin_node(fdt, child));
	CHECK(fdt_end_node(fdt));
	CHECK(fdt_end_node(fdt));
}

/*
 * Check a tree deeper than fdt_parent_offset() may track in one pass, so
 * that both it and the fallback for deeper nodes are covered
 *
 * Each level has three nodes: 'a' and 'c' each hold one subnode and 'b'
 * holds the next level. So nodes at the same depth have different parents,
 * and the walk passes whole subtrees both before and after the node being
 * looked up. The expected parent comes from the node's path, so it does not
 * depend on walking the tree
 */
static void check_deep_tree(void)
{
	int offset, depth, level, maxdepth = 0;
	char path[256];
	void *fdt = malloc(DEEP_SPACE);

	if (!fdt)
		FAIL("malloc()");

	CHECK(fdt_create(fdt, DEEP_SPACE));
	CHECK(fdt_finish_reservemap(fdt));
	CHECK(fdt_begin_node(fdt, ""));
	for (level = 0; level < DEEP_LEVELS; level++) {
		add_branch(fdt, "a", "x");
		CHECK(fdt_begin_node(fdt, "b"));
	}
	for (level = 0; level < DEEP_LEVELS; level++) {
		CHECK(fdt_end_node(fdt));
		add_branch(fdt, "c", "y");
	}
	CHECK(fdt_end_node(fdt));
	CHECK(fdt_finish(fdt));

	for (offset = 0, depth = 0; offset >= 0;
	     offset = fdt_next_node(fdt, offset, &depth)) {
		if (depth < 0)
			break;
		if (depth > maxdepth)
			maxdepth = depth;
		if (!depth)
			continue;

		CHECK(fdt_get_path(fdt, offset, path, sizeof(path)));
		check_path(fdt, path);
	}
	if (maxdepth != DEEP_LEVELS + 1)
		FAIL("tree is %d deep, expected %d", maxdepth,
		     DEEP_LEVELS + 1);
	free(fdt);
}

int main(int argc, char *argv[])
{
	void *fdt;
	int err;

	test_init(argc, argv);
	fdt = load_blob_arg(argc, argv);

	check_path(fdt, "/subnode@1");
	check_path(fdt, "/subnode@2");
	check_path(fdt, "/subnode@1/subsubnode");
	check_path(fdt, "/subnode@2/subsubnode@0");
	err = fdt_parent_offset(fdt, 0);
	if (err != -FDT_ERR_NOTFOUND)
		FAIL("fdt_parent_offset(/) returns %d instead of "
		     "-FDT_ERR_NOTFOUND", err);

	check_bad_offsets(fdt);
	check_deep_tree();

	PASS();
}
