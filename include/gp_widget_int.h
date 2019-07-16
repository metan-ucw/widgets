//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_INT_H__
#define GP_WIDGET_INT_H__

enum gp_widget_int_flags {
	GP_WIDGET_HORIZ = 0,
	GP_WIDGET_VERT = 1
};

struct gp_widget_int {
	int min, max, val;
	int alert:1;
	int dir:2;

	char payload[];
};

void gp_widget_int_set(gp_widget *self, int val);

#endif /* GP_WIDGET_INT_H__ */
