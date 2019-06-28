//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>

#include <gp_string.h>

size_t gp_string_arr_size(const char *strings[], unsigned int len)
{
	unsigned int i;
	size_t size = len * sizeof(void*);

	for (i = 0; i < len; i++)
		size += strlen(strings[i]) + 1;

	return size;
}

char **gp_string_arr_copy(const char *strings[], unsigned int len, void *buf)
{
	unsigned int i;
	char **copy = buf;

	buf += len * sizeof(void*);

	for (i = 0; i < len; i++) {
		copy[i] = buf;
		strcpy(buf, strings[i]);
		buf += strlen(strings[i]) + 1;
	}

	return copy;
}
