//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_SCROLL_AREA_H__
#define GP_WIDGET_SCROLL_AREA_H__

struct gp_widget_scroll_area {
	/* offset for the layout inside */
	gp_coord x_off;
	gp_coord y_off;

	/*
	 * If non-zero the widget minimal size is set into the stone
	 * and the content scrolls if the inner widget size is bigger.
	 */
	gp_size min_w;
	gp_size min_h;

	/* Internal do not touch */
	int scrollbar_x:1;
	int scrollbar_y:1;
	int selected:1;
	int widget_selected:1;

	gp_widget *widget;
};

gp_widget *gp_widget_scroll_area_new(gp_size min_w, gp_size min_h);

int gp_widget_scroll_area_move(gp_widget *self, gp_coord x_off, gp_coord y_off);

gp_widget *gp_widget_scroll_area_put(gp_widget *self, gp_widget *widget);

#endif /* GP_WIDGET_SCROLL_AREA_H__ */
