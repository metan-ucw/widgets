//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_FRAME_H__
#define GP_WIDGET_FRAME_H__

struct gp_widget_frame {
	gp_widget *widget;
	uint8_t has_label:1;
	uint8_t bold:1;
	char label[];
};

gp_widget *gp_widget_frame_new(const char *label, int bold, gp_widget *child);

#endif /* GP_WIDGET_FRAME_H__ */
