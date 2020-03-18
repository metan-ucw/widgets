//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_PBAR_H__
#define GP_WIDGET_PBAR_H__

enum gp_widget_pbar_type {
	GP_WIDGET_PBAR_NONE,
	GP_WIDGET_PBAR_PERCENTS,
	GP_WIDGET_PBAR_SECONDS,
	GP_WIDGET_PBAR_TMASK = 0x7f,
	GP_WIDGET_PBAR_INVERSE = 0x80,
};

struct gp_widget_pbar {
	float max;
	float val;
	enum gp_widget_pbar_type type;
};

void gp_widget_pbar_set(gp_widget *self, float val);

void gp_widget_pbar_set_max(gp_widget *self, float max);

gp_widget *gp_widget_pbar_new(float val, float max, enum gp_widget_pbar_type type);

#endif /* GP_WIDGET_PBAR_H__ */
