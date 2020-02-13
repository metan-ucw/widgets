//SPDX-License-Identifier: LGPL-2.0-or-later
/* Copyright (C) 2020 Richard Palethorpe <richiejp@f-m.fm> */

#include <stdio.h>
#include <gp_stack.h>

static void print_int_stack(int *payload)
{
	size_t i;
	gp_stack *vec = GP_STACK(payload);

	printf("gp_stack { .unit = %zu, .capacity = %zu, .length = %zu, .payload = [\n ",
	       vec->unit, vec->capacity, vec->length);

	for (i = 0; i < vec->length; i++)
	{
		printf("\t%zu\t= %d\n", i, payload[i]);
	}

	if (vec->length < vec->capacity)
		printf("\t... +%zu unused capacity\n", vec->capacity - vec->length);

	printf("]}\n");
}

int main(void)
{
	size_t i;
	int *ints = gp_stack_new(10, sizeof(int));
	char **strs;

	for (i = 0; i < GP_STACK_LEN(ints); i++)
	{
		ints[i] = i + 1;
	}

	printf("Initial range (1,10): ");
	print_int_stack(ints);

	ints = GP_STACK_GAP(ints, 0, 1);
	ints[0] = 0;

	printf("\nInsert 0: ");
	print_int_stack(ints);

	GP_STACK_PUSH(ints, 11);

	printf("\nPush 11: ");
	print_int_stack(ints);

	ints = GP_STACK_GAP(ints, 4, 1);
	ints[4] = -1;

	printf("\nInsert -1 at 4: ");
	print_int_stack(ints);

	ints = GP_STACK_GAP(ints, 5, 3);

	printf("\nInsert 3 of 0 at 5: ");
	print_int_stack(ints);

	GP_STACK_FREE(ints);

	strs = gp_stack_new(10, sizeof(char*));

	for (i = 0; i < GP_STACK_LEN(strs); i++)
	{
		strs[i] = gp_stack_new(10, sizeof(char));
		snprintf(strs[i], GP_STACK_LEN(strs[i]), "Line %zu", i);
	}

	printf("\nStack of string stacks: \n");
	for (i = 0; i < GP_STACK_LEN(strs); i++)
	{
		printf("%s\n", strs[i]);
	}

	for (i = 0; i < GP_STACK_LEN(strs); i++)
	{
		GP_STACK_FREE(strs[i]);
	}

	GP_STACK_FREE(strs);

	return 0;
}
