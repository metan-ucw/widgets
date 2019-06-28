//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <core/GP_Debug.h>
#include <gp_block_alloc.h>

static size_t block_size;

static size_t align(size_t size)
{
	size_t mask = 3;

	return (size + mask) & ~mask;
}

static gp_block *new_block(void)
{
	gp_block *block;

	if (!block_size)
		block_size = getpagesize();

	GP_DEBUG(1, "Allocating block size %zu", block_size);

	block = malloc(block_size);
	if (!block)
		return NULL;

	block->next = NULL;
	block->free = block_size - sizeof(gp_block);

	return block;
}

void gp_block_free(gp_block **self)
{
	gp_block *i, *j;

	for (i = *self; i;) {
		j = i->next;
		free(i);
		i = j;
	}

	*self = NULL;
}

static void *do_alloc(gp_block *self, size_t size)
{
	size_t pos = block_size - self->free;

	self->free -= size;

	return (char*)self + pos;
}

void *gp_block_alloc(gp_block **self, size_t size)
{
	gp_block *block;

	if (!*self) {
		GP_DEBUG(1, "Initializing empty block allocator");
		*self = new_block();
		if (!*self)
			return NULL;
	}

	size = align(size);

	for (block = *self; block; block = block->next) {
		if (block->free >= size) {
			GP_DEBUG(2, "Allocating %zu from block %p free %zu",
			         size, block, block->free);
			return do_alloc(block, size);
		}
	}

	GP_DEBUG(2, "Allocating new block size %zu", size);

	block = new_block();
	block->next = *self;
	*self = block;

	return do_alloc(block, size);
}
