//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_DIR_CACHE_H__
#define GP_DIR_CACHE_H__

#include <time.h>

typedef struct gp_dir_entry {
	size_t size;
	time_t mtime;
	int is_dir:1;
	char name[];
} gp_dir_entry;

typedef struct gp_dir_cache {
	DIR *dir;
	struct gp_block *allocator;
	size_t size;
	size_t used;
	struct gp_dir_entry *entries[];
} gp_dir_cache;

gp_dir_cache *gp_dir_cache_new(const char *path);

void gp_dir_cache_free(gp_dir_cache *self);

enum gp_dir_cache_sortType {
	GP_DIR_SORT_ASC = 0x00,
	GP_DIR_SORT_DESC = 0x04,
	GP_DIR_SORT_BY_NAME = 0x00,
	GP_DIR_SORT_BY_SIZE = 0x01,
	GP_DIR_SORT_BY_MTIME = 0x02,
};

void gp_dir_cache_sort(gp_dir_cache *self, int sort_type);

static inline gp_dir_entry *gp_dir_cache_get(gp_dir_cache *self,
                                             unsigned int pos)
{
	if (self->used <= pos)
		return NULL;

	return self->entries[pos];
}

#endif /* GP_DIR_CACHE_H__ */
