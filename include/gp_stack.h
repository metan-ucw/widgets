//SPDX-License-Identifier: LGPL-2.0-or-later
/* Copyright (C) 2020 Richard Palethorpe <richiejp@f-m.fm> */

#include <stddef.h>
#include <core/gp_common.h>

#ifndef GP_STACK_H__
#define GP_STACK_H__

/* Get a pointer to the outer gp_stack struct from a pointer to the (start of)
 * its data.
 */
#define GP_STACK(ptr) (GP_CONTAINER_OF(ptr, gp_stack, payload))

/* Get the number of elements in a stack from a pointer to its data. */
#define GP_STACK_LEN(ptr) (GP_STACK(ptr)->length)

/* Insert a gap into a stack, which is refered to by a pointer to its data ptr.
 * See gp_stack_gap().
 */
#define GP_STACK_GAP(ptr, i, len) \
	((typeof(ptr))gp_stack_gap(GP_STACK(ptr), i, len))

/* Free the stack containing ptr. */
#define GP_STACK_FREE(ptr) (free(GP_STACK(ptr)))

/* Add an element to the end of the stack. The ptr expression must resolve to
 * a variable containing a pointer to the stack data. This variable may be
 * written to with a new pointer value. Any other copies of the pointer will
 * therefor be invalidated (see gp_stack_gap).
 */
#define GP_STACK_PUSH(ptr, elem) do {\
	gp_stack *_self = GP_STACK(ptr);\
	size_t _len = _self->length;\
	ptr = gp_stack_gap(_self, _len, 1);\
	ptr[_len] = elem;\
} while(0)

/* A growable vector */
typedef struct gp_stack {
	/* No. bytes in one element in the stack */
	size_t unit;
	/* Total capacity in units */
	size_t capacity;
	/* No. of used elements in units */
	size_t length;

	char payload[];
} gp_stack;

/* Allocate a new stack and return a pointer to the data. Use the GP_STACK_*
 * macros to get or manipulate the gp_stack which data is contained in.
 */
void *gp_stack_new(size_t length, size_t unit);

/* Insert a gap into the stack of new elements, reallocating the underlying
 * memory if more capacity is needed.
 *
 * If more capacity is required, then this will reallocate the gp_stack thus
 * invalidating *self. Therefor the caller should update any pointers it has
 * to the stack data with the return value of this function.
 *
 * Newly allocated capacity, which is not within the gap, will be set to
 * 0xff. The memory within the gap will be zeroed. If allocation fails or i is
 * invalid this will return 0. The index 'i' should be <= length.
 */
void *gp_stack_gap(gp_stack *self, size_t i, size_t length);

#endif	/* GP_STACK_H */
