//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_PROGRESS_BAR_H__
#define GP_WIDGET_PROGRESS_BAR_H__

#include <gp_widget_int.h>

int gp_widget_progress_bar_set_max(gp_widget *self, int max);
int gp_widget_progress_bar_set_min(gp_widget *self, int min);
int gp_widget_progress_bar_set(gp_widget *self, int val);

gp_widget *gp_widget_progress_bar_new(int min, int max, int val);

#endif /* GP_WIDGET_PROGESS_BAR_H__ */
