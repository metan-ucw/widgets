//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_CHECKBOX_H__
#define GP_WIDGET_CHECKBOX_H__

#include <gp_widget_bool.h>

struct gp_widget *gp_widget_checkbox_new(const char *label,
                                         int set,
                                         int (*on_event)(gp_widget_event *),
                                         void *priv);

void gp_widget_checkbox_set(gp_widget *self, int val);

void gp_widget_checkbox_toggle(gp_widget *self);

#endif /* GP_WIDGET_CHECKBOX_H__ */
