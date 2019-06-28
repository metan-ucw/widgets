//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_INT_H__
#define GP_WIDGET_INT_H__

struct gp_widget_int {
	int min, max, val;
	int alert:1;

	void *event_ptr;
	int (*on_event)(gp_widget_event *);
};

gp_widget *gp_widget_int_new(int min, int max, int val,
                             int (*on_event)(gp_widget_event *), void *event_ptr);

#endif /* GP_WIDGET_INT_H__ */
