//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

#include <gp_block_alloc.h>
#include <gp_dir_cache.h>

static gp_dir_entry *new_entry(gp_dir_cache *self, size_t size,
                               const char *name, mode_t mode, time_t mtime)
{
	size_t name_len = strlen(name);
	size_t entry_size;
	int is_dir = 0;
	gp_dir_entry *entry;

	if ((mode & S_IFMT) == S_IFDIR)
		is_dir = 1;

	entry_size = sizeof(gp_dir_entry) + name_len + is_dir + 1;

	entry = gp_block_alloc(&self->allocator, entry_size);
	if (!entry)
		return NULL;

	entry->size = size;
	entry->is_dir = is_dir;
	sprintf(entry->name, "%s%s", name, is_dir ? "/" : "");
	entry->mtime = mtime;

	return entry;
}

static void put_entry(gp_dir_cache **self, gp_dir_entry *entry)
{
	if ((*self)->used >= (*self)->size) {
		gp_dir_cache *new;
		size_t new_size;

		new_size = sizeof(gp_dir_cache);
		new_size += ((*self)->size + 50) * sizeof(void*);

		new = realloc(*self, new_size);
		if (!new)
			return;

		new->size += 50;
		*self = new;
	}

	(*self)->entries[(*self)->used++] = entry;
}

static void populate(gp_dir_cache **self)
{
	for (;;) {
		gp_dir_entry *entry;
		struct dirent *ent;
		struct stat buf;

		ent = readdir((*self)->dir);
		if (!ent)
			return;

		if (!strcmp(ent->d_name, ".") ||
		    !strcmp(ent->d_name, ".."))
			continue;

		if (stat(ent->d_name, &buf))
			return;

		entry = new_entry(*self, buf.st_size, ent->d_name,
		                  buf.st_mode, buf.st_mtim.tv_sec);
		if (!entry)
			return;

		put_entry(self, entry);
	}
}

static int cmp_asc_name(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	return strcmp((*ea)->name, (*eb)->name);
}

static int cmp_desc_name(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	return strcmp((*eb)->name, (*ea)->name);
}

static int cmp_asc_size(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	if ((*ea)->size == (*eb)->size)
		return 0;

	return (*ea)->size > (*eb)->size;
}

static int cmp_desc_size(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	if ((*ea)->size == (*eb)->size)
		return 0;

	return (*ea)->size < (*eb)->size;
}

static int cmp_asc_time(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	if ((*ea)->mtime == (*eb)->mtime)
		return 0;

	return (*ea)->mtime > (*eb)->mtime;
}

static int cmp_desc_time(const void *a, const void *b)
{
	const gp_dir_entry *const *ea = a;
	const gp_dir_entry *const *eb = b;

	if ((*ea)->mtime == (*eb)->mtime)
		return 0;

	return (*ea)->mtime < (*eb)->mtime;
}

static int (*cmp_funcs[])(const void *, const void *) = {
	[GP_DIR_SORT_ASC  | GP_DIR_SORT_BY_NAME] = cmp_asc_name,
	[GP_DIR_SORT_DESC | GP_DIR_SORT_BY_NAME] = cmp_desc_name,
	[GP_DIR_SORT_ASC  | GP_DIR_SORT_BY_SIZE] = cmp_asc_size,
	[GP_DIR_SORT_DESC | GP_DIR_SORT_BY_SIZE] = cmp_desc_size,
	[GP_DIR_SORT_ASC  | GP_DIR_SORT_BY_MTIME] = cmp_asc_time,
	[GP_DIR_SORT_DESC | GP_DIR_SORT_BY_MTIME] = cmp_desc_time,
};

void gp_dir_cache_sort(gp_dir_cache *self, int sort_type)
{
	int (*cmp_func)(const void *, const void *) = cmp_funcs[sort_type];

	if (!cmp_func)
		return;

	qsort(self->entries, self->used, sizeof(void*), cmp_func);
}

gp_dir_cache *gp_dir_cache_new(const char *path)
{
	DIR *dir;
	gp_dir_cache *ret;

	dir = opendir(path);
	if (!dir)
		return NULL;

	ret = malloc(sizeof(gp_dir_cache) + 100 * sizeof(void*));
	if (!ret) {
		closedir(dir);
		return NULL;
	}

	ret->dir = dir;
	ret->size = 100;
	ret->used = 0;
	ret->allocator = NULL;

	populate(&ret);

	gp_dir_cache_sort(ret, 0);

	return ret;
}

void gp_dir_cache_free(gp_dir_cache *self)
{
	closedir(self->dir);
	gp_block_free(&self->allocator);
	free(self);
}

