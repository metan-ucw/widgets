//SPDX-License-Identifier: LGPL-2.0-or-later
/* Copyright (C) 2020 Richard Palethorpe <richiejp@f-m.fm> */

#include <string.h>
#include <gp_stack.h>

static void *expand(gp_stack *self, size_t length)
{
	size_t capacity = GP_MAX(2, self->capacity);

	while (self->length + length >= capacity)
		capacity *= 2;

	self = realloc(self, sizeof(gp_stack) + capacity * self->unit);
	if (!self)
		return 0;

	memset(self->payload + self->length * self->unit, 0xff,
	       (capacity - self->length) * self->unit);

	self->capacity = capacity;
	self->length += length;

	return self;
}

void *gp_stack_new(size_t length, size_t unit)
{
	gp_stack *self = malloc(sizeof(gp_stack));

	if (!self)
		return 0;

	memset(self, 0, sizeof(gp_stack));
	self->unit = unit;

	self = expand(self, length);
	if (!self)
		return 0;

	return (void *)self->payload;
}

void *gp_stack_gap(gp_stack *self, size_t i, size_t length)
{
	if (i > self->length)
		return 0;

	self = expand(self, length);

	if (!self)
		return 0;

	if (i >= self->length - length)
		goto out;

	memmove(self->payload + (i + length) * self->unit,
		self->payload + i * self->unit,
		(self->length - length - i) * self->unit);

	memset(self->payload + i * self->unit, 0, length * self->unit);
out:
	return (void *)self->payload;
}
