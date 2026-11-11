// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * fdtdump.c - Contributed by Pantelis Antoniou <pantelis.antoniou AT gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>

#include "util.h"

#define FDT_MAGIC_SIZE	4
#define MAX_VERSION 17U

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)	((void *)(ALIGN((uintptr_t)(p), (a))))

static const char *tagname(uint32_t tag)
{
	static const char * const names[] = {
#define TN(t) [t] = #t
		TN(FDT_BEGIN_NODE),
		TN(FDT_END_NODE),
		TN(FDT_PROP),
		TN(FDT_NOP),
		TN(FDT_END),
#undef TN
	};
	if (tag < ARRAY_SIZE(names))
		if (names[tag])
			return names[tag];
	return "FDT_???";
}

#define dumpf(fmt, args...) \
	do { if (debug) printf("// " fmt, ## args); } while (0)

static uint32_t next_cell(const void *blob, uint32_t *off, uint32_t limit)
{
	const uint32_t *p;

	*off += sizeof(uint32_t);
	if (*off >= limit)
		die("** End of blob while reading cell\n");
	p = (const uint32_t *)((const char *)blob + *off) - 1;
	return fdt32_to_cpu(*p);
}

static void dump_blob(void *blob, bool debug)
{
	struct fdt_header *bph = blob;
	uint32_t off_mem_rsvmap = fdt32_to_cpu(bph->off_mem_rsvmap);
	uint32_t off_dt = fdt32_to_cpu(bph->off_dt_struct);
	uint32_t off_str = fdt32_to_cpu(bph->off_dt_strings);
	uint32_t version = fdt32_to_cpu(bph->version);
	uint32_t totalsize = fdt32_to_cpu(bph->totalsize);
	uint32_t tag;
	uint32_t o;
	int depth, shift;
	int i;
	uint64_t addr, size;

	depth = 0;
	shift = 4;

	printf("/dts-v1/;\n");
	printf("// magic:\t\t0x%"PRIx32"\n", fdt32_to_cpu(bph->magic));
	printf("// totalsize:\t\t0x%"PRIx32" (%"PRIu32")\n",
	       totalsize, totalsize);
	printf("// off_dt_struct:\t0x%"PRIx32"\n", off_dt);
	printf("// off_dt_strings:\t0x%"PRIx32"\n", off_str);
	printf("// off_mem_rsvmap:\t0x%"PRIx32"\n", off_mem_rsvmap);
	printf("// version:\t\t%"PRIu32"\n", version);
	printf("// last_comp_version:\t%"PRIu32"\n",
	       fdt32_to_cpu(bph->last_comp_version));
	if (version >= 2)
		printf("// boot_cpuid_phys:\t0x%"PRIx32"\n",
		       fdt32_to_cpu(bph->boot_cpuid_phys));

	if (version >= 3)
		printf("// size_dt_strings:\t0x%"PRIx32"\n",
		       fdt32_to_cpu(bph->size_dt_strings));
	if (version >= 17)
		printf("// size_dt_struct:\t0x%"PRIx32"\n",
		       fdt32_to_cpu(bph->size_dt_struct));
	printf("\n");

	for (i = 0; ; i++) {
		struct fdt_reserve_entry *re;
		uint32_t off = off_mem_rsvmap + i * sizeof(*re);

		if (off + sizeof(*re) > totalsize) {
			printf("// Memory reservation block out of bounds\n");
			break;
		}

		re = (struct fdt_reserve_entry *)((char *)blob + off);
		addr = fdt64_to_cpu(re->address);
		size = fdt64_to_cpu(re->size);
		if (addr == 0 && size == 0)
			break;

		printf("/memreserve/ %#"PRIx64" %#"PRIx64";\n",
		       addr, size);
	}

	o = off_dt;
	while ((tag = next_cell(blob, &o, totalsize)) != FDT_END) {

		dumpf("%04"PRIx32": tag: 0x%08"PRIx32" (%s)\n",
		        o - 4, tag, tagname(tag));

		if (tag == FDT_BEGIN_NODE) {
			const char *p = (const char *)blob + o;
			const char *e = memchr(p, '\0', totalsize - o);
			size_t namelen;

			if (!e)
				die("** End of blob in node name\n");

			namelen = e - p;

			o = ALIGN(o + namelen + 1, 4);

			if (*p == '\0')
				p = "/";

			printf("%*s%s {\n", depth * shift, "", p);

			depth++;
			continue;
		}

		if (tag == FDT_END_NODE) {
			depth--;

			printf("%*s};\n", depth * shift, "");
			continue;
		}

		if (tag == FDT_NOP) {
			printf("%*s// [NOP]\n", depth * shift, "");
			continue;
		}

		if (tag == FDT_PROP) {
			uint32_t sz = next_cell(blob, &o, totalsize);
			uint32_t off = off_str + next_cell(blob, &o, totalsize);
			uint32_t valo;
			const char *s;

			if (version < 16 && sz >= 8)
				o = ALIGN(o, 8);
			valo = o;
			o = ALIGN(o + sz, 4);

			if (off >= totalsize) {
				s = "<out of bounds>";
			} else {
				s = (const char *)blob + off;
				if (!memchr(s, '\0', totalsize-off))
					s = "<unterminated>";
			}
			dumpf("%04"PRIx32": string: %s\n", off, s);
			dumpf("%04"PRIx32": value\n", valo);
			printf("%*s%s", depth * shift, "", s);
			if (valo + sz > totalsize)
				die("** End of blob in property value\n");
			utilfdt_print_data((const char *)blob + valo, sz);
			printf(";\n");
			continue;
		}

		die("** Unknown tag 0x%08"PRIx32"\n", tag);
	}
}

/* Usage related data. */
static const char usage_synopsis[] = "fdtdump [options] <file>";
static const char usage_short_opts[] = "ds" USAGE_COMMON_SHORT_OPTS;
static struct option const usage_long_opts[] = {
	{"debug",            no_argument, NULL, 'd'},
	{"scan",             no_argument, NULL, 's'},
	USAGE_COMMON_LONG_OPTS
};
static const char * const usage_opts_help[] = {
	"Dump debug information while decoding the file",
	"Scan for an embedded fdt in file",
	USAGE_COMMON_OPTS_HELP
};

static bool valid_header(char *p, size_t len)
{
	if (len < sizeof(struct fdt_header) ||
	    fdt_magic(p) != FDT_MAGIC ||
	    fdt_last_comp_version(p) > MAX_VERSION ||
	    fdt_totalsize(p) >= len ||
	    fdt_off_dt_struct(p) >= len ||
	    fdt_off_dt_strings(p) >= len)
		return 0;
	else
		return 1;
}

int main(int argc, char *argv[])
{
	int opt;
	const char *file;
	char *buf;
	bool debug = false;
	bool scan = false;
	size_t len;

	fprintf(stderr, "\n"
"**** fdtdump is a low-level debugging tool, not meant for general use.\n"
"**** If you want to decompile a dtb, you probably want\n"
"****     dtc -I dtb -O dts <filename>\n\n"
		);
	while ((opt = util_getopt_long()) != EOF) {
		switch (opt) {
		case_USAGE_COMMON_FLAGS

		case 'd':
			debug = true;
			break;
		case 's':
			scan = true;
			break;
		}
	}
	if (optind != argc - 1)
		usage("missing input filename");
	file = argv[optind];

	buf = utilfdt_read(file, &len);
	if (!buf)
		die("could not read: %s\n", file);

	/* try and locate an embedded fdt in a bigger blob */
	if (scan) {
		unsigned char smagic[FDT_MAGIC_SIZE];
		char *p = buf;
		char *endp = buf + len;

		fdt32_st(smagic, FDT_MAGIC);

		/* poor man's memmem */
		while ((endp - p) >= FDT_MAGIC_SIZE) {
			p = memchr(p, smagic[0], endp - p - FDT_MAGIC_SIZE);
			if (!p)
				break;
			if (fdt_magic(p) == FDT_MAGIC) {
				/* try and validate the main struct */
				off_t this_len = endp - p;
				if (valid_header(p, this_len))
					break;
				if (debug)
					printf("%s: skipping fdt magic at offset %#tx\n",
						file, p - buf);
			}
			++p;
		}
		if (!p || (size_t)(endp - p) < sizeof(struct fdt_header))
			die("%s: could not locate fdt magic\n", file);
		printf("%s: found fdt at offset %#tx\n", file, p - buf);
		buf = p;
	} else if (!valid_header(buf, len))
		die("%s: header is not valid\n", file);

	dump_blob(buf, debug);

	return 0;
}
