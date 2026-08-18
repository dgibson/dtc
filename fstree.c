// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright David Gibson <dwg@au1.ibm.com>, IBM Corporation.  2005.
 */

#include "dtc.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

static bool is_reserved_name(const char *name)
{
	return streq(name, ".") || streq(name, "..");
}

static bool is_safe_name(const char *name)
{
	return !is_reserved_name(name) && !strchr(name, '/');
}

static struct node *read_fstree(const char *dirname)
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	struct node *tree;

	d = opendir(dirname);
	if (!d)
		die("Couldn't opendir() \"%s\": %s\n", dirname, strerror(errno));

	tree = build_node(NULL, NULL, NULL);

	while ((de = readdir(d)) != NULL) {
		char *tmpname;

		if (is_reserved_name(de->d_name))
			continue;

		tmpname = join_path(dirname, de->d_name);

		if (stat(tmpname, &st) < 0)
			die("stat(%s): %s\n", tmpname, strerror(errno));

		if (S_ISREG(st.st_mode)) {
			struct property *prop;
			FILE *pfile;

			pfile = fopen(tmpname, "rb");
			if (! pfile) {
				fprintf(stderr,
					"WARNING: Cannot open %s: %s\n",
					tmpname, strerror(errno));
			} else {
				prop = build_property(de->d_name,
						      data_copy_file(pfile,
								     st.st_size),
						      NULL);
				add_property(tree, prop);
				fclose(pfile);
			}
		} else if (S_ISDIR(st.st_mode)) {
			struct node *newchild;

			newchild = read_fstree(tmpname);
			newchild = name_node(newchild, xstrdup(de->d_name));
			add_child(tree, newchild);
		}

		free(tmpname);
	}

	closedir(d);
	return tree;
}

static void write_fstree(int dirfd, struct node *tree)
{
	struct property *prop;
	struct node *child;
	int fd, rc;
	FILE *pfile;

	for_each_property(tree, prop) {
		if (!is_safe_name(prop->name))
			continue;

		fd = openat(dirfd, prop->name, O_WRONLY | O_CREAT | O_TRUNC, 0666);

		if (fd < 0)
			die("Couldn't openat(%d, \"%s\"): %s\n", dirfd,
			    prop->name, strerror(errno));
		pfile = fdopen(fd, "wb");
		if (fwrite(prop->val.val, prop->val.len, 1, pfile) != 1)
			die("Couldn't write to \"%s\"\n", prop->name);
		fclose(pfile);
	}
	for_each_child(tree, child) {
		if (!is_safe_name(child->name))
			continue;

		rc = mkdirat(dirfd, child->name, 0777);
		if (rc < 0 && errno != EEXIST)
			goto fail_dir;
		fd = openat(dirfd, child->name, O_DIRECTORY | O_NOFOLLOW);
		if (fd < 0)
fail_dir:
			die("Couldn't mkdirat/openat(%d, \"%s\"): %s\n", dirfd,
			    child->name, strerror(errno));
		write_fstree(fd, child);
		close(fd);
	}
}

struct dt_info *dt_from_fs(const char *dirname)
{
	struct node *tree;

	tree = read_fstree(dirname);
	tree = name_node(tree, "");

	return build_dt_info(DTSF_V1, NULL, tree, guess_boot_cpuid(tree));
}

void dt_to_fs(const char *dirname, struct dt_info *dti)
{
	int dirfd, rc;

	rc = mkdir(dirname, 0777);
	if (rc < 0 && errno != EEXIST)
		goto fail;
	dirfd = open(dirname, O_DIRECTORY);
	if (dirfd < 0)
fail:
		die("Couldn't open/mkdir(\"%s\"): %s\n", dirname, strerror(errno));

	write_fstree(dirfd, dti->dt);
}
