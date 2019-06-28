//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_BLOCK_ALLOC_H__
#define GP_BLOCK_ALLOC_H__

typedef struct gp_block {
	struct gp_block *next;
	size_t free;
} gp_block;

void *gp_block_alloc(gp_block **self, size_t size);

void gp_block_free(gp_block **self);

#endif /* GP_BLOCK_ALLOC_H__ */
