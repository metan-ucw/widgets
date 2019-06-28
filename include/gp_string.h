//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_STRING_H__
#define GP_STRING_H__

/**
 * @brief Computes sum of sizes needed for a deep copy of an array of strings.
 *
 * @param strings An array of strings.
 * @param len Length of the string array.
 * @return Size needed for a deep copy of the string array.
 */
size_t gp_string_arr_size(const char *strings[], unsigned int len);

/**
 * @brief Copies an array of strings into a buffer.
 *
 * @param strings An array of strings.
 * @param len Length of the string array.
 * @buf Buffer large enough to fit the copy.
 * @return A deep copy of the string array.
 */
char **gp_string_arr_copy(const char *strings[], unsigned int len, void *buf);

#endif /* GP_STRING_H__ */
