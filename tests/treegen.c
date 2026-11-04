// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * treegen - generate test device tree blobs
 *
 * Build example device tree blobs for test.  Note that some of the generated
 * blobs are deliberately malformed, in order to test error conditions.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <libfdt_env.h>
#include <fdt.h>

#include "tests.h"
#include "testdata.h"

/*
 * Buffer with write cursor.  Emit helpers write at the cursor, advance it, and
 * return the offset at which data was written.
 */
struct buf {
	char *data;
	size_t size, cursor;
};

#define CHUNKSIZE	1024

static struct buf buf_init(void)
{
	struct buf b = { NULL, 0, 0 };

	return b;
}

/* Low-level helpers */

static size_t emit_bytes(struct buf *b, const void *data, int len)
{
	size_t off = b->cursor;

	if (!len)
		return off;

	if (b->cursor + len > b->size) {
		size_t newsize = b->cursor + len + CHUNKSIZE;

		b->data = xrealloc(b->data, newsize);
		memset(b->data + b->size, 0, newsize - b->size);
		b->size = newsize;
	}
	memcpy(b->data + b->cursor, data, len);
	b->cursor += len;
	return off;
}

static size_t emit_u32(struct buf *b, uint32_t val)
{
	fdt32_t beval = cpu_to_fdt32(val);

	return emit_bytes(b, &beval, sizeof(beval));
}

static size_t emit_u64(struct buf *b, uint64_t val)
{
	fdt64_t beval = cpu_to_fdt64(val);

	return emit_bytes(b, &beval, sizeof(beval));
}

static size_t emit_string(struct buf *b, const char *str)
{
	return emit_bytes(b, str, strlen(str) + 1);
}

#define MAX_ALIGNMENT	8

static size_t emit_align(struct buf *b, size_t alignment)
{
	size_t pad = ((b->cursor + alignment - 1) & ~(alignment - 1)) - b->cursor;
	static const char zeros[MAX_ALIGNMENT];

	assert(alignment <= MAX_ALIGNMENT);
	assert(pad < alignment);

	return emit_bytes(b, zeros, pad);
}

static size_t start_block(struct buf *b)
{
	return b->cursor;
}

static void fill_prop_name(struct buf *b, size_t strs, size_t prop, size_t stroff)
{
	struct fdt_property *fp = (struct fdt_property *)(b->data + prop);

	fp->nameoff = cpu_to_fdt32(stroff - strs);
}

/* FDT structure helpers */

static size_t emit_fdt_header(struct buf *b)
{
	size_t off = emit_u32(b, FDT_MAGIC);

	emit_u32(b, 0);	/* totalsize */
	emit_u32(b, 0);	/* off_dt_struct */
	emit_u32(b, 0);	/* off_dt_strings */
	emit_u32(b, 0);	/* off_mem_rsvmap */
	emit_u32(b, 0x11);	/* version */
	emit_u32(b, 0x10);	/* last_comp_version */
	emit_u32(b, 0);	/* boot_cpuid_phys */
	emit_u32(b, 0);	/* size_dt_strings */
	emit_u32(b, 0);	/* size_dt_struct */
	return off;
}

static void finish_rsvmap(struct buf *b, size_t hdr, size_t rsvmap)
{
	struct fdt_header *fh = (struct fdt_header *)(b->data + hdr);

	fh->off_mem_rsvmap = cpu_to_fdt32(rsvmap - hdr);
}

static void finish_struct_block(struct buf *b, size_t hdr, size_t start)
{
	struct fdt_header *fh = (struct fdt_header *)(b->data + hdr);

	fh->off_dt_struct = cpu_to_fdt32(start - hdr);
	fh->size_dt_struct = cpu_to_fdt32(b->cursor - start);
}

static void finish_strings_block(struct buf *b, size_t hdr, size_t start)
{
	struct fdt_header *fh = (struct fdt_header *)(b->data + hdr);

	fh->off_dt_strings = cpu_to_fdt32(start - hdr);
	fh->size_dt_strings = cpu_to_fdt32(b->cursor - start);
}

static void finish_totalsize(struct buf *b, size_t hdr)
{
	struct fdt_header *fh = (struct fdt_header *)(b->data + hdr);

	fh->totalsize = cpu_to_fdt32(b->cursor - hdr);
}

static size_t emit_rsvmap_entry(struct buf *b, uint64_t addr, uint64_t size)
{
	size_t off = emit_u64(b, addr);

	emit_u64(b, size);
	return off;
}

static size_t emit_rsvmap_term(struct buf *b)
{
	return emit_rsvmap_entry(b, 0, 0);
}

static void emit_rsvmap_empty(struct buf *b, size_t hdr)
{
	size_t rsvmap = start_block(b);

	emit_rsvmap_term(b);
	finish_rsvmap(b, hdr, rsvmap);
}

static size_t emit_begin_node(struct buf *b, const char *name)
{
	size_t off = emit_u32(b, FDT_BEGIN_NODE);

	emit_string(b, name);
	emit_align(b, FDT_TAGSIZE);
	return off;
}

static size_t emit_end_node(struct buf *b)
{
	return emit_u32(b, FDT_END_NODE);
}

static size_t emit_fdt_end(struct buf *b)
{
	return emit_u32(b, FDT_END);
}

static size_t emit_prop_header(struct buf *b, int nameoff, int len)
{
	size_t off = emit_u32(b, FDT_PROP);

	emit_u32(b, len);
	emit_u32(b, nameoff);
	return off;
}

static size_t emit_prop_nil(struct buf *b, int nameoff)
{
	return emit_prop_header(b, nameoff, 0);
}

static size_t emit_prop_u32(struct buf *b, int nameoff, uint32_t val)
{
	size_t off = emit_prop_header(b, nameoff, 4);

	emit_u32(b, val);
	return off;
}

static size_t emit_prop_u64(struct buf *b, int nameoff, uint64_t val)
{
	size_t off = emit_prop_header(b, nameoff, 8);

	emit_u64(b, val);
	return off;
}

static size_t emit_prop_str(struct buf *b, int nameoff, const char *str)
{
	int len = strlen(str) + 1;
	size_t off = emit_prop_header(b, nameoff, len);

	emit_bytes(b, str, len);
	emit_align(b, FDT_TAGSIZE);
	return off;
}

#define emit_prop_strings(b, nameoff, ...) \
	emit_prop_strings_(b, nameoff, __VA_ARGS__, NULL)

static size_t emit_prop_strings_(struct buf *b, int nameoff, ...)
{
	size_t off = emit_prop_header(b, nameoff, 0);
	size_t start = b->cursor;
	va_list ap;
	const char *s;

	va_start(ap, nameoff);
	while ((s = va_arg(ap, const char *)) != NULL)
		emit_string(b, s);
	va_end(ap);

	((struct fdt_property *)(b->data + off))->len =
		cpu_to_fdt32(b->cursor - start);
	emit_align(b, FDT_TAGSIZE);
	return off;
}

/*
 * Tree generators
 *
 * Each function builds a complete DTB in a buffer.
 */

static struct buf make_test_tree1(void)
{
	struct buf b = buf_init();
	size_t hdr;
	size_t p_compatible, p_prop_int, p_prop_int64, p_prop_str,
	       p_address_cells, p_size_cells;
	size_t p_s1_compatible, p_s1_reg, p_s1_prop_int;
	size_t p_s1ss_compatible, p_s1ss_placeholder, p_s1ss_prop_int;
	size_t p_s2_reg, p_s2_linux_phandle, p_s2_prop_int,
	       p_s2_address_cells, p_s2_size_cells;
	size_t p_s2ss0_reg, p_s2ss0_phandle, p_s2ss0_compatible,
	       p_s2ss0_prop_int;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	{
		size_t rsvmap = start_block(&b);

		emit_rsvmap_entry(&b, TEST_ADDR_1, TEST_SIZE_1);
		emit_rsvmap_entry(&b, TEST_ADDR_2, TEST_SIZE_2);
		emit_rsvmap_term(&b);
		finish_rsvmap(&b, hdr, rsvmap);
	}

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		p_compatible = emit_prop_str(&b, 0, "test_tree1");
		p_prop_int = emit_prop_u32(&b, 0, TEST_VALUE_1);
		p_prop_int64 = emit_prop_u64(&b, 0, TEST_VALUE64_1);
		p_prop_str = emit_prop_str(&b, 0, TEST_STRING_1);
		p_address_cells = emit_prop_u32(&b, 0, 1);
		p_size_cells = emit_prop_u32(&b, 0, 0);

		emit_begin_node(&b, "subnode@1");
		p_s1_compatible = emit_prop_str(&b, 0, "subnode1");
		p_s1_reg = emit_prop_u32(&b, 0, 1);
		p_s1_prop_int = emit_prop_u32(&b, 0, TEST_VALUE_1);

		emit_begin_node(&b, "subsubnode");
		p_s1ss_compatible = emit_prop_strings(&b, 0,
						     "subsubnode1", "subsubnode");
		p_s1ss_placeholder = emit_prop_strings(&b, 0,
						       "this is a placeholder string",
						       "string2");
		p_s1ss_prop_int = emit_prop_u32(&b, 0, TEST_VALUE_1);
		emit_end_node(&b);

		emit_begin_node(&b, "ss1");
		emit_end_node(&b);

		emit_end_node(&b);

		emit_begin_node(&b, "subnode@2");
		p_s2_reg = emit_prop_u32(&b, 0, 2);
		p_s2_linux_phandle = emit_prop_u32(&b, 0, PHANDLE_1);
		p_s2_prop_int = emit_prop_u32(&b, 0, TEST_VALUE_2);
		p_s2_address_cells = emit_prop_u32(&b, 0, 1);
		p_s2_size_cells = emit_prop_u32(&b, 0, 0);

		emit_begin_node(&b, "subsubnode@0");
		p_s2ss0_reg = emit_prop_u32(&b, 0, 0);
		p_s2ss0_phandle = emit_prop_u32(&b, 0, PHANDLE_2);
		p_s2ss0_compatible = emit_prop_strings(&b, 0,
						      "subsubnode2", "subsubnode");
		p_s2ss0_prop_int = emit_prop_u32(&b, 0, TEST_VALUE_2);
		emit_end_node(&b);

		emit_begin_node(&b, "ss2");
		emit_end_node(&b);

		emit_end_node(&b);

		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);
		size_t s;

		s = emit_string(&b, "compatible");
		fill_prop_name(&b, strs, p_compatible, s);
		fill_prop_name(&b, strs, p_s1_compatible, s);
		fill_prop_name(&b, strs, p_s1ss_compatible, s);
		fill_prop_name(&b, strs, p_s2ss0_compatible, s);

		s = emit_string(&b, "prop-int");
		fill_prop_name(&b, strs, p_prop_int, s);
		fill_prop_name(&b, strs, p_s1_prop_int, s);
		fill_prop_name(&b, strs, p_s1ss_prop_int, s);
		fill_prop_name(&b, strs, p_s2_prop_int, s);
		fill_prop_name(&b, strs, p_s2ss0_prop_int, s);

		fill_prop_name(&b, strs, p_prop_int64,
			       emit_string(&b, "prop-int64"));

		fill_prop_name(&b, strs, p_prop_str,
			       emit_string(&b, "prop-str"));

		fill_prop_name(&b, strs, p_s2_linux_phandle,
			       emit_string(&b, "linux,phandle"));

		fill_prop_name(&b, strs, p_s2ss0_phandle,
			       emit_string(&b, "phandle"));

		s = emit_string(&b, "reg");
		fill_prop_name(&b, strs, p_s1_reg, s);
		fill_prop_name(&b, strs, p_s2_reg, s);
		fill_prop_name(&b, strs, p_s2ss0_reg, s);

		fill_prop_name(&b, strs, p_s1ss_placeholder,
			       emit_string(&b, "placeholder"));

		s = emit_string(&b, "#address-cells");
		fill_prop_name(&b, strs, p_address_cells, s);
		fill_prop_name(&b, strs, p_s2_address_cells, s);

		s = emit_string(&b, "#size-cells");
		fill_prop_name(&b, strs, p_size_cells, s);
		fill_prop_name(&b, strs, p_s2_size_cells, s);

		finish_strings_block(&b, hdr, strs);
	}

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_truncated_property(void)
{
	struct buf b = buf_init();
	size_t hdr;
	size_t p;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		p = emit_prop_header(&b, 0, 4);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);

		emit_string(&b, "truncated");
		fill_prop_name(&b, strs, p, strs);
		finish_strings_block(&b, hdr, strs);
	}

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_bad_node_char(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_begin_node(&b, "sub$node");
		emit_end_node(&b);
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_bad_node_format(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_begin_node(&b, "subnode@1@2");
		emit_end_node(&b);
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_bad_prop_char(void)
{
	struct buf b = buf_init();
	size_t hdr;
	size_t p;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		p = emit_prop_u32(&b, 0, TEST_VALUE_1);
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);

		fill_prop_name(&b, strs, p,
			       emit_string(&b, "prop$erty"));
		finish_strings_block(&b, hdr, strs);
	}

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_ovf_size_strings(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_prop_u32(&b, 0x10000000, 0);
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);

		emit_string(&b, "x");
		finish_strings_block(&b, hdr, strs);
	}

	((struct fdt_header *)(b.data + hdr))->size_dt_strings =
		cpu_to_fdt32(0xffffffff);

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_truncated_string(void)
{
	struct buf b = buf_init();
	size_t hdr;
	size_t p_good, p_bad;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		p_good = emit_prop_nil(&b, 0);
		p_bad = emit_prop_nil(&b, 0);
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);

		fill_prop_name(&b, strs, p_good, emit_string(&b, "good"));
		fill_prop_name(&b, strs, p_bad, emit_bytes(&b, "bad", 3));
		finish_strings_block(&b, hdr, strs);
	}

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_truncated_memrsv(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	{
		size_t rsvmap;

		emit_align(&b, 8);
		rsvmap = start_block(&b);
		emit_rsvmap_entry(&b, TEST_ADDR_1, TEST_SIZE_1);
		finish_rsvmap(&b, hdr, rsvmap);
	}

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_unterminated_memrsv(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);

	{
		size_t rsvmap;

		emit_align(&b, 8);
		rsvmap = start_block(&b);
		emit_rsvmap_entry(&b, TEST_ADDR_1, TEST_SIZE_1);
		finish_rsvmap(&b, hdr, rsvmap);
	}

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_two_roots(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		emit_end_node(&b);
		emit_begin_node(&b, "");
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_named_root(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "fake");
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	finish_strings_block(&b, hdr, start_block(&b));

	finish_totalsize(&b, hdr);

	return b;
}

static struct buf make_bad_proplen(void)
{
	struct buf b = buf_init();
	size_t hdr;

	hdr = emit_fdt_header(&b);
	emit_align(&b, 8);

	emit_rsvmap_empty(&b, hdr);

	{
		size_t ss = start_block(&b);

		emit_begin_node(&b, "");
		/* Property with length exceeding the remaining blob size */
		emit_u32(&b, FDT_PROP);
		emit_u32(&b, 0x80000000);	/* proplen: exceeds blob */
		emit_u32(&b, 0);		/* nameoff */
		emit_end_node(&b);
		emit_fdt_end(&b);
		finish_struct_block(&b, hdr, ss);
	}

	{
		size_t strs = start_block(&b);

		emit_string(&b, "x");
		finish_strings_block(&b, hdr, strs);
	}

	finish_totalsize(&b, hdr);

	return b;
}

/* Tree table and main */

static struct {
	struct buf (*generator)(void);
	const char *filename;
} trees[] = {
#define TREE(name)	{ make_##name, #name ".dtb" }
	TREE(test_tree1),
	TREE(bad_node_char), TREE(bad_node_format), TREE(bad_prop_char),
	TREE(ovf_size_strings),
	TREE(truncated_property), TREE(truncated_string),
	TREE(truncated_memrsv),
	TREE(unterminated_memrsv),
	TREE(two_roots),
	TREE(named_root),
	TREE(bad_proplen),
};

int main(int argc, char *argv[])
{
	unsigned int i;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <output-dir>\n", argv[0]);
		return 1;
	}

	if (chdir(argv[1]) != 0) {
		perror("chdir()");
		return 1;
	}

	for (i = 0; i < ARRAY_SIZE(trees); i++) {
		struct buf b = trees[i].generator();
		const char *filename = trees[i].filename;
		int fd;
		ssize_t ret;

		printf("Tree \"%s\", %zu bytes\n", filename, b.cursor);

		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0)
			perror("open()");

		ret = write(fd, b.data, b.cursor);
		if (ret < 0 || (size_t)ret != b.cursor)
			perror("write()");

		close(fd);
		free(b.data);
	}

	return 0;
}
