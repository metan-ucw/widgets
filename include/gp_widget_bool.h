//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_BOOL_H__
#define GP_WIDGET_BOOL_H__

struct gp_widget_bool {
	const char *label;
	int val;
	void *event_ptr;
	int (*on_event)(gp_widget_event *ev);
	char payload[];
};

#endif /* GP_WIDGET_BOOL_H__ */
