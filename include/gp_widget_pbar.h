//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_PBAR_H__
#define GP_WIDGET_PBAR_H__

struct gp_widget_pbar {
	float val; /* in percents */
};

void gp_widget_pbar_set(gp_widget *self, float val);

gp_widget *gp_widget_pbar_new(float val);

#endif /* GP_WIDGET_PBAR_H__ */
