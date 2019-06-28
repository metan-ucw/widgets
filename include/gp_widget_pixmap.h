//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_PIXMAP_H__
#define GP_WIDGET_PIXMAP_H__

struct gp_widget_pixmap {
	int (*on_event)(gp_widget_event *ev);
	unsigned int w, h;
	gp_pixmap *pixmap;
	void *event_ptr;
};

struct gp_widget *gp_widget_pixmap_new(unsigned int w, unsigned int h,
                                       int (*on_event)(gp_widget_event *ev),
                                       void *event_ptr);

#endif /* GP_WIDGET_PIXMAP_H__ */
